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

struct scene {
    // 纯资产池assets
    std::vector<sphere> spheres;
    std::vector<quad> quads;
    std::vector<triangle> triangles;
    std::vector<mesh> meshes;

    // 带变换实例池
    std::vector<instance> instances;

    // 场景渲染列表
    // 包括带变换instance和基础图元(不带变换实例)
    std::vector<AssetRef> objects;

    std::vector<scene_bvh_node> nodes;
    bool is_bvh_built = false;

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
    void add(const sphere& s) { add(add_asset(s)); }
    void add(const quad& q) { add(add_asset(q)); }
    void add(const triangle& t) { add(add_asset(t)); }
    void add(const mesh& m) { add(add_asset(m)); }
    void add(const AssetRef& ref) {
        objects.push_back(ref);
        is_bvh_built = false;
    }

    // 带变换添加图元, 内部自动包装 Instance, 会被渲染
    void add(const sphere& s, const mat4& transform) { add(add_asset(s), transform); }
    void add(const quad& q, const mat4& transform) { add(add_asset(q), transform); }
    void add(const triangle& t, const mat4& transform) { add(add_asset(t), transform); }
    void add(const mesh& m, const mat4& transform) { add(add_asset(m), transform); }

    // 用已有资产加变换添加, 包装成 Instance, 会被渲染
    void add(const AssetRef& ref, const mat4& transform) {
        int inst_idx = add_instance_impl(ref.type, ref.index, transform);
        objects.push_back({ PRIM_INSTANCE, inst_idx });
    }


    void build_BVH() {
        nodes.clear();
        if (objects.empty()) {
            is_bvh_built = true;
            return;
        }

        std::vector<scene_prim_ref> prims;
        for (int i = 0; i < (int)objects.size(); ++i) {
            int type = objects[i].type;
            int index = objects[i].index;
            aabb local_bounds;
            bool valid = true;

            if (type == PRIM_INSTANCE) {
                const auto& inst = instances[index];
                switch (inst.prim_type) {
                case PRIM_SPHERE:   local_bounds = spheres[inst.prim_index].get_aabb(); break;
                case PRIM_QUAD:     local_bounds = quads[inst.prim_index].get_aabb(); break;
                case PRIM_TRIANGLE: local_bounds = triangles[inst.prim_index].get_aabb(); break;
                case PRIM_MESH: {
                    const mesh& m = meshes[inst.prim_index];
                    if (!m.is_bvh_built) valid = false;
                    local_bounds = m.get_aabb();
                    break;
                }
                default: valid = false;
                }

                if (valid) {
                    aabb world_bounds;
                    for (int k = 0; k < 8; ++k) {
                        vec3 pt(k & 1 ? local_bounds.max.x() : local_bounds.min.x(),
                            k & 2 ? local_bounds.max.y() : local_bounds.min.y(),
                            k & 4 ? local_bounds.max.z() : local_bounds.min.z());
                        world_bounds.expand(inst.transform.transform_point(pt));
                    }
                    prims.push_back({ world_bounds, PRIM_INSTANCE, index });
                }
            }
            else {
                // 直接图元
                switch (type) {
                case PRIM_SPHERE:   local_bounds = spheres[index].get_aabb(); break;
                case PRIM_QUAD:     local_bounds = quads[index].get_aabb(); break;
                case PRIM_TRIANGLE: local_bounds = triangles[index].get_aabb(); break;
                case PRIM_MESH: {
                    const mesh& m = meshes[index];
                    if (!m.is_bvh_built) valid = false;
                    local_bounds = m.get_aabb();
                    break;
                }
                default: valid = false;
                }
                if (valid) prims.push_back({ local_bounds, type, index });
            }
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