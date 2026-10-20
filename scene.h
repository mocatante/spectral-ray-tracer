#pragma once
#include <vector>
#include "hittable.h"
#include "mat4.h"

#define LEAF_NODE -1

// 三角形引用, 辅助构建BVH 
struct prim_ref {
    aabb bounds;
    int index;      // 在 triangles 数组里的索引
};

struct bvh_node {
    aabb bounds;
    int left_idx;   // -1 = 叶子
    int right_idx;
    int triangle_idx;  // 叶子三角形在triangles 数组里的索引

    bool is_leaf() const { return left_idx == LEAF_NODE; }
};


struct mesh {
    std::vector<triangle> triangles;
    std::vector<bvh_node> nodes;

    bool is_bvh_built = false;

    aabb get_aabb() const {
        if (!nodes.empty()) return nodes[0].bounds;
        return aabb();
    }

    void add(const triangle& t) {
        triangles.push_back(t);
        is_bvh_built = false;
    }


    int build_recursive(std::vector<prim_ref>& prims, int start, int end) {
        int node_idx = (int)nodes.size();
        nodes.push_back(bvh_node());

        // 计算当前区间的总AABB
        aabb bounds;
        for (int i = start; i < end; ++i) bounds.expand(prims[i].bounds);
        nodes[node_idx].bounds = bounds;

        int count = end - start;
		
        // 叶子
        if (count == 1) {
            nodes[node_idx].left_idx = LEAF_NODE;
            nodes[node_idx].right_idx = LEAF_NODE;
            nodes[node_idx].triangle_idx = prims[start].index;
            return node_idx;
        }
		// 计算选择最长轴
        aabb center_bounds;
        for (int i = start; i < end; ++i) {
            center_bounds.expand(prims[i].bounds.center());
        }
        float ex = center_bounds.max.x() - center_bounds.min.x();
        float ey = center_bounds.max.y() - center_bounds.min.y();
        float ez = center_bounds.max.z() - center_bounds.min.z();
        int axis = (ey > ex && ey > ez) ? 1 : ((ez > ex && ez > ey) ? 2 : 0);

        // 中位数切分
        int mid = start + count / 2;
        std::nth_element(prims.begin() + start, prims.begin() + mid, prims.begin() + end,
            [axis](const prim_ref& a, const prim_ref& b) {
                return a.bounds.center()[axis] < b.bounds.center()[axis];
            });

        int left_child = build_recursive(prims, start, mid);
        int right_child = build_recursive(prims, mid, end);

        nodes[node_idx].left_idx = left_child;
        nodes[node_idx].right_idx = right_child;
        return node_idx;
    }

	// 构建 BVH
    void build_BVH() {
        nodes.clear();
        if (triangles.empty()) {
            // is_bvh_built = false;
            return;
        }

        // 构建 prims，用于建树时的空间排序
        std::vector<prim_ref> prims;
        prims.reserve(triangles.size());
        for (int i = 0; i < (int)triangles.size(); ++i) {
            aabb b = triangles[i].get_aabb();
            prims.push_back(prim_ref{ b,i});
        }

        // 预分配节点空间
        nodes.reserve(prims.size() * 2 - 1);

        // 递归建树
        build_recursive(prims, 0, (int)prims.size());

        is_bvh_built = true;
    }


    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
        if (!is_bvh_built || nodes.empty()) {
            return false;   // 不做暴力渲染
        }

        bool hit_anything = false;
        float closest_so_far = t_max;

        int stack[64];
        int stack_ptr = 0;
        stack[stack_ptr++] = 0; // 根节点下标为0

        while (stack_ptr > 0) {
            int node_idx = stack[--stack_ptr];
            const bvh_node& node = nodes[node_idx];

            // 包围盒测试
            if (!node.bounds.hit(r, t_min, closest_so_far)) continue;

            // 叶子节点
            if (node.is_leaf()) {
                hit_record temp_rec;
                if (triangles[node.triangle_idx].hit(r, t_min, closest_so_far, temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }
            // 内部节点, 左右孩子入栈
            else {
                stack[stack_ptr++] = node.left_idx;
                stack[stack_ptr++] = node.right_idx;
            }
        }
        return hit_anything;
    }
};

enum PrimType : int {
    PRIM_SPHERE = 0,
    PRIM_QUAD = 1,
    PRIM_TRIANGLE = 2,
    PRIM_MESH = 3,
    PRIM_INSTANCE = 4
};

// 仅提供变换包装, 无hit
struct instance {
    int prim_type;
    int prim_index;
    mat4 transform;
    mat4 inv_transform;
    mat4 normal_matrix;
};


// 用于辅助建立BVH
struct scene_prim_ref {
    aabb bounds;
    int type;
    int index;
};

struct scene_bvh_node {
    aabb bounds;
    int left_idx;   // -1 = 叶子
    int right_idx;
    int prim_type;  // 叶子的图元类型
    int prim_index; // 叶子的图元索引

    bool is_leaf() const { return left_idx == LEAF_NODE; }
};

// 资产引用
// 用于场景构建时返回
// 告诉外部scene添加了什么
// 以复用资产
struct AssetRef {
    int type;
    int index;
};

// 辅助对物体直接采样
enum DirectSampleFlag : int {
    NOT_DIRECT_SAMPLE = 0,
    IS_DIRECT_SAMPLE = 1
};

struct DirectTargetRef {
    AssetRef ref;
    float weight;
    Point bound_center; // 外接球，用于复杂形体粗采样
    float bound_radius;
};


// 场景设计为 资产和实例分离
// 纯资产池Asset: 不参与渲染
// 变换实例池Instances: 显式带变换实例
// 场景渲染列表Objects: 会被建入 BVH 并被渲染的基础图元和Instance(统一包装成AssetRef)
//
// 对于不带变换的图元，不会为其显式创建 Instance 包装。
// 它们直接作为 AssetRef 进入 objects 列表
// 从逻辑上讲，视为"隐藏单位矩阵的特殊实例"
struct scene {
    // 纯资产池Assets
    std::vector<sphere> spheres;
    std::vector<quad> quads;
    std::vector<triangle> triangles;
    std::vector<mesh> meshes;

    // 带变换实例池
    std::vector<instance> instances;

    // 场景渲染列表
    // 包括带变换instance和基础图元(不带变换实例)
    std::vector<AssetRef> objects;

    // 直接采样相关
    std::vector<DirectTargetRef> direct_targets;
    float total_target_weight = 0.0f;

    std::vector<scene_bvh_node> nodes;
    bool is_bvh_built = false;


    bool has_targets() const { return !direct_targets.empty() && total_target_weight > 0.0f; }

    // 添加资产, 不会被渲染
    AssetRef add_asset(const sphere& s) {
        int idx = (int)spheres.size();
        spheres.push_back(s);
        return { PRIM_SPHERE, idx };
    }
    AssetRef add_asset(const quad& q) {
        int idx = (int)quads.size();
        quads.push_back(q);
        return { PRIM_QUAD, idx };
    }
    AssetRef add_asset(const triangle& t) {
        int idx = (int)triangles.size();
        triangles.push_back(t);
        return { PRIM_TRIANGLE, idx };
    }
    AssetRef add_asset(const mesh& m) {
        int idx = (int)meshes.size();
        meshes.push_back(m);
        return { PRIM_MESH, idx };
    }

    // 直接添加图元, 升级为不带变换实例, 会被渲染
    void add(const sphere& s, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(s), flag, weight); }
    void add(const quad& q, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(q), flag, weight); }
    void add(const triangle& t, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(t), flag, weight); }
    void add(const mesh& m, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(m), flag, weight); }
    void add(const AssetRef& ref, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(ref, flag, weight); }

    // 带变换添加图元, 内部自动包装 Instance, 会被渲染
    void add(const sphere& s, const mat4& transform, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(s), transform, flag, weight); }
    void add(const quad& q, const mat4& transform, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(q), transform, flag, weight); }
    void add(const triangle& t, const mat4& transform, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(t), transform, flag, weight); }
    void add(const mesh& m, const mat4& transform, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(add_asset(m), transform, flag, weight); }
    // 用已有资产加变换添加, 包装成 Instance, 会被渲染
    void add(const AssetRef& ref, const mat4& transform, DirectSampleFlag flag = NOT_DIRECT_SAMPLE, float weight = 1.0f) { add_impl(ref, transform, flag, weight); }

    // 直接采样方向
    // 前置条件: 调用方通过 has_targets() 保证 direct_targets 非空且 total_target_weight > 0
    void sample_direct_target(const Point& origin, Direction& out_dir) const {
        // 加权随机选一个
        float r = random_float() * total_target_weight;
        int picked = 0;
        for (int i = 0; i < (int)direct_targets.size(); ++i) {
            r -= direct_targets[i].weight;
            if (r <= 0.0f) { picked = i; break; }
        }
        sample_primitive(direct_targets[picked], origin, out_dir);
    }
    // 直接采样对应pdf
    float pdf_direct_target(const Point& origin, const Direction& dir) const {
        float total_pdf = 0.0f;
        for (const auto& t : direct_targets) {
            float p = pdf_primitive(t, origin, dir);
            total_pdf += (t.weight / total_target_weight) * p;
        }
        return total_pdf;
    }


    void build_BVH() {
        nodes.clear();
        if (objects.empty()) {
            is_bvh_built = true;
            return;
        }

        std::vector<scene_prim_ref> prims;
        for (int i = 0; i < (int)objects.size(); ++i) {
            aabb bounds = get_primitive_aabb(objects[i]);
            if (bounds.min.x() > bounds.max.x()) continue;
            prims.push_back({ bounds, objects[i].type, objects[i].index });
        }
        if (prims.empty()) {
            is_bvh_built = true;
            return;
        }

        // 预分配节点空间
        nodes.reserve(prims.size() * 2 - 1);

        // 递归建树
        build_recursive(prims, 0, (int)prims.size());

        is_bvh_built = true;
    }

    bool hit_primitive(int type, int index, const ray& r, float t_min, float t_max, hit_record& rec) const {
        switch (type) {
        case PRIM_SPHERE:   return spheres[index].hit(r, t_min, t_max, rec);
        case PRIM_QUAD:     return quads[index].hit(r, t_min, t_max, rec);
        case PRIM_TRIANGLE: return triangles[index].hit(r, t_min, t_max, rec);
        case PRIM_MESH:     return meshes[index].hit(r, t_min, t_max, rec);

        case PRIM_INSTANCE: {
            // 变换光线到局部空间求交
            const instance& inst = instances[index];
            vec3 local_o = inst.inv_transform.transform_point(r.origin);
            vec3 local_d = inst.inv_transform.transform_vector(r.direction);
            ray local_ray(local_o, local_d);
            hit_record local_rec;
            if (hit_primitive(inst.prim_type, inst.prim_index, local_ray, t_min, t_max, local_rec)) {
                // 信息变换到世界空间
                rec = local_rec;
                rec.p = inst.transform.transform_point(local_rec.p);
                rec.normal = inst.normal_matrix.transform_vector(local_rec.normal).normalize();
                return true;
            }
            return false;
        }
        default: return false;
        }
    }

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
        if (!is_bvh_built || nodes.empty()) {
            return false;   //不做暴力渲染
        }

        bool hit_anything = false;
        float closest_so_far = t_max;

        int stack[64];
        int stack_ptr = 0;
        stack[stack_ptr++] = 0;

        while (stack_ptr > 0) {
            int node_idx = stack[--stack_ptr];
            const scene_bvh_node& node = nodes[node_idx];

            if (!node.bounds.hit(r, t_min, closest_so_far)) continue;

            if (node.is_leaf()) {
                hit_record temp_rec;
                if (hit_primitive(node.prim_type, node.prim_index, r, t_min, closest_so_far, temp_rec)) {
                    hit_anything = true;
                    closest_so_far = temp_rec.t;
                    rec = temp_rec;
                }
            }
            else {
                stack[stack_ptr++] = node.left_idx;
                stack[stack_ptr++] = node.right_idx;
            }
        }
        return hit_anything;
    }

private:
    // 辅助将变换包装成 Instance
    int add_instance_impl(int type, int index, const mat4& transform) {
        instance inst;
        inst.prim_type = type;
        inst.prim_index = index;
        inst.transform = transform;
        inst.inv_transform = transform.inverse();
        inst.normal_matrix = inst.inv_transform.transpose();

        instances.push_back(inst);
        is_bvh_built = false;
        return (int)instances.size() - 1;
    }

    // 注册target
    void register_target(const AssetRef& ref, float weight) {
        aabb b = get_primitive_aabb(ref);
        if (b.min.x() > b.max.x()) return;
        direct_targets.push_back({ ref, weight, b.center(), b.get_sphere_radius() });
        total_target_weight += weight;
    }

    void add_impl(const AssetRef& ref, DirectSampleFlag isTarget, float weight) {
        objects.push_back(ref);
        if (isTarget == IS_DIRECT_SAMPLE) register_target(ref, weight);
        is_bvh_built = false;
    }

    void add_impl(const AssetRef& ref, const mat4& transform, DirectSampleFlag isTarget, float weight) {
        int inst_idx = add_instance_impl(ref.type, ref.index, transform);
        AssetRef inst_ref = { PRIM_INSTANCE, inst_idx };
        objects.push_back(inst_ref);
        if (isTarget == IS_DIRECT_SAMPLE) register_target(inst_ref, weight);
    }

    // 注册直接采样目标
    void register_direct_target(const AssetRef& ref, float weight) {
        aabb b = get_primitive_aabb(ref);
        if (b.min.x() > b.max.x()) return;
        direct_targets.push_back({ ref, weight, b.center(), b.get_sphere_radius() });
        total_target_weight += weight;
    }

    // hittable的精确pdf采样
    // 如果未实现, 则使用aabb外接球粗略采样
    // 更新hittable的pdf后需要手动更新此处switch
    // 注意, 
    // 约定图元的sample需要保证失败时退化为其它采样
    void sample_primitive(const DirectTargetRef& t, const Point& origin, Direction& out_dir) const {
        switch (t.ref.type) {
        case PRIM_SPHERE: {
            spheres[t.ref.index].sample(origin, out_dir);
            break;
        }
        case PRIM_QUAD: {
            quads[t.ref.index].sample(origin, out_dir);
            break;
        }
        // 当新增精确pdf时添加
        default:
            sample_bounding_sphere(t, origin, out_dir);
        }
    }

    // hittable报告的精确采样概率
    // 未实现则使用aabb外接球
    // 更新hittable的pdf后需要手动更新此处switch
    float pdf_primitive(const DirectTargetRef& t, const Point& origin, const Direction& dir) const {
        switch (t.ref.type) {
        case PRIM_SPHERE:   return spheres[t.ref.index].pdf(origin, dir);
        case PRIM_QUAD:     return quads[t.ref.index].pdf(origin, dir);
        // 当新增精确pdf时添加
        default:
            return pdf_bounding_sphere(t, origin, dir);
        }
    }

    // aabb外接球采样
    void sample_bounding_sphere(const DirectTargetRef& t, const Point& origin, Direction& out_dir) const {
        vec3 to_center = t.bound_center - origin;
        float dist2 = to_center.dot(to_center);
        // 直接采样失败, 退化为均匀球面采样
        if (t.bound_radius <= 1e-6f || dist2 <= t.bound_radius * t.bound_radius) {
            out_dir = random_unit_vector();
            return;
        }

        vec3 w = to_center.normalize();
        vec3 u, v;
        build_onb(w, u, v);

        float cos_theta_max = std::sqrt(std::max(0.0f, 1.0f - (t.bound_radius * t.bound_radius) / dist2));
        float cos_theta = 1.0f - random_float() * (1.0f - cos_theta_max);
        float sin_theta = std::sqrt(std::max(0.0f, 1.0f - cos_theta * cos_theta));
        float phi = 2.0f * PI_F * random_float();

        vec3 local_dir(sin_theta * std::cos(phi), sin_theta * std::sin(phi), cos_theta);
        out_dir = (u * local_dir.x() + v * local_dir.y() + w * local_dir.z()).normalize();
    }

    // aabb外接球采样概率
    float pdf_bounding_sphere(const DirectTargetRef& t, const Point& origin, const Direction& dir) const {
        vec3 to_center = t.bound_center - origin;
        float dist2 = to_center.dot(to_center);
        // 退化情况, 均匀球面采样概率
        if (t.bound_radius <= 1e-6f || dist2 <= t.bound_radius * t.bound_radius) {
            return INV_FOUR_PI;
        }

        float cos_theta_max = std::sqrt(std::max(0.0f, 1.0f - (t.bound_radius * t.bound_radius) / dist2));
        float solid_angle = 2.0f * PI_F * (1.0f - cos_theta_max);
        float cos_angle = to_center.normalize().dot(dir);
        if (cos_angle > cos_theta_max - 1e-4f) {
            return 1.0f / solid_angle;
        }
        return 0.0f;
    }


    // 世界空间aabb
    aabb get_primitive_aabb(const AssetRef& ref) const {
        if (ref.type == PRIM_INSTANCE) {
            const instance& inst = instances[ref.index];
            aabb local_b = get_local_aabb(inst.prim_type, inst.prim_index);
            if (local_b.min.x() > local_b.max.x()) return aabb();

            aabb world_b;
            for (int k = 0; k < 8; ++k) {
                vec3 pt(k & 1 ? local_b.max.x() : local_b.min.x(),
                    k & 2 ? local_b.max.y() : local_b.min.y(),
                    k & 4 ? local_b.max.z() : local_b.min.z());
                world_b.expand(inst.transform.transform_point(pt));
            }
            return world_b;
        }

        return get_local_aabb(ref.type, ref.index);
    }

    aabb get_local_aabb(int type, int index) const {
        switch (type) {
        case PRIM_SPHERE:   return spheres[index].get_aabb();
        case PRIM_QUAD:     return quads[index].get_aabb();
        case PRIM_TRIANGLE: return triangles[index].get_aabb();
        case PRIM_MESH:     return meshes[index].get_aabb();
        default: return aabb();
        }
    }


    int build_recursive(std::vector<scene_prim_ref>& prims, int start, int end) {
        int node_idx = (int)nodes.size();
        nodes.push_back(scene_bvh_node());

        aabb bounds;
        for (int i = start; i < end; ++i) bounds.expand(prims[i].bounds);
        nodes[node_idx].bounds = bounds;

        int count = end - start;
        if (count == 1) {
            nodes[node_idx].left_idx = LEAF_NODE;
            nodes[node_idx].right_idx = LEAF_NODE;
            nodes[node_idx].prim_type = prims[start].type;
            nodes[node_idx].prim_index = prims[start].index;
            return node_idx;
        }

        aabb center_bounds;
        for (int i = start; i < end; ++i) center_bounds.expand(prims[i].bounds.center());

        float ex = center_bounds.max.x() - center_bounds.min.x();
        float ey = center_bounds.max.y() - center_bounds.min.y();
        float ez = center_bounds.max.z() - center_bounds.min.z();
        int axis = (ey > ex && ey > ez) ? 1 : ((ez > ex && ez > ey) ? 2 : 0);

        int mid = start + count / 2;
        std::nth_element(prims.begin() + start, prims.begin() + mid, prims.begin() + end,
            [axis](const scene_prim_ref& a, const scene_prim_ref& b) {
                return a.bounds.center()[axis] < b.bounds.center()[axis];
            });

        int left_child = build_recursive(prims, start, mid);
        int right_child = build_recursive(prims, mid, end);

        nodes[node_idx].left_idx = left_child;
        nodes[node_idx].right_idx = right_child;
        return node_idx;
    }
};