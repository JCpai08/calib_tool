# CircleDetector::detect 流程说明

本文整理 [core/CircleDetector.cpp](core/CircleDetector.cpp#L280) 中 `CircleDetector::detect(const cv::Mat &image)` 的执行流程，并结合 [core/CircleDetector.h](core/CircleDetector.h#L23) 里的 `CircleDetectionConfig` 说明各参数的作用。

本文只描述当前默认会执行的分支。对于 `if` 条件为 `false` 的分支，例如 `useHough = false`，不展开说明。

## 1. 输入检查

对应代码首先判断输入图像是否为空：

- 如果 `image.empty()` 为真，直接返回空结果。
- 这一步是整个检测流程的入口保护，避免后续 OpenCV 操作对空图像报错。

## 2. 图像转灰度

如果输入图像是三通道图像，方法会先通过 `cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY)` 转成灰度图；否则直接复用原图。

这一阶段的目标是统一后续检测输入，使阈值化、轮廓分析和强度统计都基于单通道图像进行。

相关实现：

- `detect` 中完成输入灰度化。
- `binaryForDarkMarkers`、`passesIntensityFilter`、`refineWithEllipseFitting` 内部也会再次检查通道数，确保函数在直接调用时同样安全。

## 3. 生成暗标记二值图

`detect` 会调用 `binaryForDarkMarkers(gray)`，把灰度图转换成适合找暗色圆点的二值图。

### 3.1 CLAHE 局部对比度增强

函数先使用 CLAHE 对灰度图做自适应直方图均衡，增强局部对比度。这样可以缓解光照不均导致的标记边缘不明显问题。

### 3.2 高斯模糊

随后使用 `cv::GaussianBlur` 做平滑，抑制噪点，减少阈值化后的碎片区域。

### 3.3 自适应阈值

再调用 `cv::adaptiveThreshold`，参数为：

- 阈值类型：`cv::ADAPTIVE_THRESH_GAUSSIAN_C`
- 二值化类型：`cv::THRESH_BINARY_INV`
- 邻域大小：`31`
- 常量偏移：`6`

这里使用反向二值化，是因为目标是提取暗色标记。

### 3.4 形态学去噪

最后使用 3x3 椭圆核进行开运算和闭运算，去除孤立噪点并填补局部断裂。

### 3.5 调试输出

该函数会把处理后的灰度图存入 `m_debugGray`，把二值图存入 `m_debugBinary`，供外部调试查看。

## 4. 基于轮廓提取候选圆

`detect` 接着调用 `fromContours(binary)`，从二值图中提取候选圆点。

### 4.1 轮廓搜索

函数通过 `cv::findContours(binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE)` 获取全部轮廓。

### 4.2 面积过滤

根据图像面积计算轮廓面积范围：

- 最小面积 = 图像面积 x `minAreaRatio`
- 最大面积 = 图像面积 x `maxAreaRatio`

当前默认值来自 `CircleDetectionConfig`：

- `minAreaRatio = 1e-6`
- `maxAreaRatio = 3e-3`

这个阶段会过滤过小的噪声轮廓和过大的背景区域。

### 4.3 圆度过滤

对每个轮廓计算：

- 周长：`cv::arcLength`
- 圆度：`4 * PI * area / perimeter^2`

只有圆度不低于 `minCircularity` 的轮廓才会继续保留。默认值为 `0.72`。

### 4.4 最小外接圆和半径过滤

通过 `cv::minEnclosingCircle` 获取候选圆心和半径，再结合以下配置过滤：

- `minRadiusPx = 3.0`
- `maxRadiusPx = 80.0`

### 4.5 组装 CirclePoint

通过上述筛选后，会构造 `CirclePoint`，并写入：

- `x`, `y`：圆心坐标
- `radius`：半径
- `area`：轮廓面积
- `circularity`：圆度
- `confidence`：初始值 `0.5`
- `source`：`contour`

### 4.6 半径一致性二次筛选

当候选点集不为空时，函数会计算所有候选点的平均半径，然后保留半径不小于平均半径一半的点。

这一步的目标是去掉明显偏小、质量较差的候选项。

### 4.7 置信度重算

最后调用 `calcConfidence(filtered)` 重新计算置信度。

其逻辑是：

- 先统计所有候选点的平均圆度和标准差
- 再根据每个点的圆度相对分布计算 `circScore`
- 结合圆度本身计算 `areaScore`
- 最终按 `0.6 * circScore + 0.4 * areaScore` 更新 `confidence`

## 5. Hough 分支跳过

`detect` 中还有一个 `if (m_config.useHough)` 分支，用于从 `fromHough(gray)` 补充圆检测结果。

当前 `CircleDetectionConfig` 默认值为：

- `useHough = false`

因此这条分支在当前流程里不执行，本文直接跳过。

## 6. 强度一致性过滤

轮廓候选点生成后，`detect` 会遍历 `markers`，对每个点调用 `passesIntensityFilter(gray, marker)`。

这个阶段的目标是验证目标区域是否符合“中心更暗、外圈更亮”的特征。

### 6.1 局部 ROI 选取

函数会以当前候选圆心和半径为中心，构造一个边长约为 4 倍半径的局部 ROI。

如果 ROI 为空，直接返回 `false`。

### 6.2 距离掩码构建

函数逐像素计算当前点到候选圆心的平方距离，形成距离掩码，后续据此把区域拆分为内圈和环带。

### 6.3 内圈和环带统计

使用以下固定比例区间统计灰度均值：

- 内圈：半径的 0.75 倍以内
- 环带内边界：半径的 1.1 倍
- 环带外边界：半径的 1.8 倍

统计得到：

- `innerMean`：中心区域平均灰度
- `ringMean`：外环平均灰度

### 6.4 过滤条件

函数要求：

- 内圈像素数量至少为 12
- 环带像素数量至少为 20
- `innerMean <= maxInnerIntensity`
- `ringMean - innerMean >= minRingContrast`

当前默认值为：

- `maxInnerIntensity = 170.0`
- `minRingContrast = 18.0`

如果不满足，候选点会被丢弃。

## 7. 合并相近候选点

通过强度过滤后，`detect` 调用 `mergeCloseMarkers(filtered)` 合并距离过近的重复候选。

### 7.1 按半径排序

函数先按半径从大到小排序，优先保留更大的候选点。

### 7.2 距离去重

随后逐个检查候选点和已保留点的欧氏距离：

- 如果距离小于等于 `mergeDistancePx`，则认为是同一个目标，当前点丢弃
- 否则保留

当前默认值：

- `mergeDistancePx = 8.0`

### 7.3 最终排序

合并后会再按位置排序：

- 如果两个点的 `y` 差值大于 10，则按 `y` 升序排
- 否则按 `x` 升序排

### 7.4 来源标记

函数会把所有保留点的 `source` 设置为 `merged`。

## 8. 椭圆细化

`detect` 中还会判断 `if (m_config.useEllipseRefinement)`。

当前默认值为：

- `useEllipseRefinement = true`

因此这一阶段会执行，并调用 `refineWithEllipseFitting(image, marker)` 对合并后的候选点做进一步精修。

### 8.1 裁剪局部区域

函数会以当前圆心、半径和 `ellipseMargin` 为基础，在原图中裁剪一个局部区域。

默认值：

- `ellipseMargin = 5.0`

如果裁剪范围无效，直接返回 `false`。

### 8.2 二值化和轮廓提取

裁剪图会做 Otsu 反向阈值化，然后再次 `findContours`。

该阶段的目标是从局部区域中提取更稳定的目标边界，用于椭圆拟合。

### 8.3 取最大轮廓

函数会从所有轮廓中选择面积最大的那个作为拟合目标。

如果轮廓点数少于 `ellipseMinContourPoints`，则直接失败。

默认值：

- `ellipseMinContourPoints = 5`

### 8.4 椭圆拟合与几何约束

使用 `cv::fitEllipse` 拟合椭圆，并计算：

- 轴比 `ratio`
- 圆度 `circularity`
- 实心度 `solidity`

然后与下面的配置比较：

- `ellipseMinRatio = 0.7`
- `ellipseMaxRatio = 1.4`
- `ellipseMinCircularity = 0.7`
- `ellipseMinSolidity = 0.8`

只有全部满足时，当前候选点才会被认为有效。

### 8.5 更新位置、来源和置信度

如果拟合成功，函数会：

- 用椭圆中心更新 `marker.x` 和 `marker.y`
- 将 `source` 设为 `ellipse_refined`
- 根据圆度、实心度和轴比，通过 `calcMarkerConfidence` 重新计算 `confidence`

其中 `calcMarkerConfidence` 的权重为：

- 圆度占 40%
- 实心度占 40%
- 轴比得分占 20%

## 9. 统一输出整理

流程最后会遍历 `merged`，把每个点的 `source` 重新写成 `contour`，同时递增内部计数器 `m_idCounterGlobal`。

最终返回的是经过以下步骤筛选后的结果：

1. 输入有效性检查
2. 灰度化
3. 暗标记二值化
4. 轮廓候选提取
5. 强度过滤
6. 距离合并
7. 椭圆细化
8. 最终输出

## 10. 相关配置项速查

以下参数都来自 [core/CircleDetector.h](core/CircleDetector.h#L23)：

- `minAreaRatio`：候选轮廓最小面积占比，默认 `1e-6`
- `maxAreaRatio`：候选轮廓最大面积占比，默认 `3e-3`
- `minCircularity`：轮廓圆度下限，默认 `0.72`
- `minRadiusPx`：最小半径，默认 `3.0`
- `maxRadiusPx`：最大半径，默认 `80.0`
- `mergeDistancePx`：候选点合并距离阈值，默认 `8.0`
- `useHough`：是否启用 Hough 分支，默认 `false`
- `houghDp` / `houghMinDist` / `houghParam1` / `houghParam2`：Hough 圆检测参数
- `maxInnerIntensity`：中心区域最大平均灰度，默认 `170.0`
- `minRingContrast`：环带与中心的最小灰度差，默认 `18.0`
- `ellipseMargin`：椭圆细化裁剪边距，默认 `5.0`
- `ellipseMinRatio` / `ellipseMaxRatio`：椭圆轴比范围
- `ellipseMinCircularity`：椭圆细化时的圆度下限
- `ellipseMinSolidity`：椭圆细化时的实心度下限
- `ellipseMinContourPoints`：椭圆拟合所需的最少轮廓点数
- `useEllipseRefinement`：是否启用椭圆细化，默认 `true`
- `debug`：是否输出调试信息，默认 `false`

## 11. 总结

`detect` 的整体思路可以概括为：先把图像变成适合找暗圆点的二值图，再通过轮廓筛选出候选圆，接着用灰度强度关系和空间距离去除低质量重复项，最后在默认开启的椭圆细化阶段进一步修正位置和置信度。