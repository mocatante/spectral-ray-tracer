# spectral-ray-tracer
A minimalist C++ spectral path tracer based on RTIOW, featuring BVH, Cauchy dispersion ,Direct Light Sampling and CUDA-ready structs.
基于 RTIOW 改造的极简 C++ 光谱路径追踪器，包含BVH, 柯西色散, 直接光采样与面向CUDA的struct设计。
## preview/预览
<img width="1260" height="709" alt="preview2" src="https://github.com/user-attachments/assets/763a6817-6614-4723-9095-3a94720bdec5" />
<img width="1260" height="709" alt="preview1" src="https://github.com/user-attachments/assets/3cf967f3-9939-4b4d-9591-3e34022cc87d" />
<img width="1260" height="709" alt="ecbe21d283776bfcf69a70ad25159540" src="https://github.com/user-attachments/assets/25f15ae4-aeee-4907-bbfc-85a6d821ba36" />
<img width="800" height="800" alt="preview3" src="https://github.com/user-attachments/assets/e949368f-78ee-434c-915d-66edc9af79e0" />



## deepwiki
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/mocatante/spectral-ray-tracer)

## Features/特性
- **Spectral Rendering**:95 channels (360nm–830nm).
  光谱渲染：95 个通道( 360nm–830nm )
- **BVH**:with a simple BVH built.
  实现简单的BVH遍历
- **Cauchy Dispersion**: Accurate glass refraction.
  利用柯西色散实现色散效果
- **Mixture Importance Sampling (MIS)**：Combines material BSDF sampling and direct target sampling with a 50/50 mixture probability.
  混合重要性采样：以 50% 概率混合直接光采样与材质散射，降低噪声
- **TLAS/BLAS Architecture**: Strictly separates scene assets from instances. Transform-free primitives bypass the instance wrapper for optimal memory and traversal performance.
  资产/实例分离架构：场景资产与实例严格分离，无变换的图元跳过 Instance 包装，优化内存与遍历性能。
- **CUDA Ready**: All primitives and materials use `struct`.
  面向 CUDA, 使用struct设计, 方便未来迁移至 GPU

## Build & Run / 编译运行
Requires C++17 and OpenMP.Or you can just delete the line at camera.h
```
#pragma omp parallel for schedule(dynamic, 1)
```
需要 C++17 及 OpenMP 支持。


## Design Choice/设计选择

### 关于优化
Only simle BVH and Direct Light Sampling, for a basic accelerate. Considering that more efficient ready-made library will be used when migrating to CUDA in the future.
仅简单实现类BVH和直接光采样, 考虑到未来迁移CUDA时将使用现成的更高效的库.

### The kB Matrix/关于 kB 矩阵
Converting 3 RGB values to a 95-channel spectrum is a highly underdetermined problem.
Instead of manually solving linear equations, I wrote a quick PyTorch script (rgbToLambda.py) and let gradient descent figure it out.
It balances the conflicting constraints of albedo [0,1] and emission >1. 
This introduces a slight cyan tint, but it's good enough for visualization.
将 3 个 RGB 值转换为 95 个波长的光谱是典型的欠定问题
理论上应该算方程组可行解, 在挑选最优解. 但我选择用梯度下降的方式快速训练一个.
为了同时兼容反照率 [0,1] 和发光度 >1, 模型做了妥协, 导致画面微微泛青, 但是渲染效果看够用

## Credits/致谢
This project is heavily inspired by the book series *Ray Tracing in One Weekend* by Peter Shirley.
Peter Shirley, Trevor David Black, Steve Hollasch.
*Ray Tracing in One Weekend*.
Version 4.0.2, 2025-04-25.
https://raytracing.github.io/books/RayTracingInOneWeekend.html
