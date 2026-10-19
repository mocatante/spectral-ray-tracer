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
            is_bvh_built = true;
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

struct instance {
    int mesh_index;
    mat4 transform;
    mat4 inv_transform;
    mat4 normal_matrix;

    bool hit(const ray& r, float t_min, float t_max, hit_record& rec, const std::vector<mesh>& prototypes) const {
		// 变换光线到局部空间
        vec3 local_o = inv_transform.transform_point(r.origin);
        vec3 local_d = inv_transform.transform_vector(r.direction);
        ray local_ray(local_o, local_d);

        hit_record temp_rec;
        if (prototypes[mesh_index].hit(local_ray, t_min, t_max, temp_rec)) {
			// 命中信息变换回世界空间
            rec = temp_rec;
            rec.p = transform.transform_point(temp_rec.p);
            rec.normal = normal_matrix.transform_vector(temp_rec.normal).normalize();
            return true;
        }
        return false;
    }
};


enum PrimType : int {
    PRIM_SPHERE = 0,
    PRIM_QUAD = 1,
    PRIM_TRIANGLE = 2,
    PRIM_INSTANCE = 3
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



struct scene {
    std::vector<sphere> spheres;
    std::vector<quad> quads;
    std::vector<triangle> triangles;
    std::vector<mesh> prototypes;
    std::vector<instance> instances;

    std::vector<scene_bvh_node> nodes;
    bool is_bvh_built = false;

    void add(const sphere& s) { spheres.push_back(s); is_bvh_built = false; }
    void add(const quad& q) { quads.push_back(q); is_bvh_built = false; }
    void add(const triangle& t) { triangles.push_back(t); is_bvh_built = false; }
    // 注意返回索引
    // 在scene中, mesh仅作为资产
    // instance需要明确指向一个mesh
    // 对应关系需要在scene外部维护
    int add_mesh(const mesh& m) {
        int idx = (int)prototypes.size();
        prototypes.push_back(m);
        return idx;
    }
    void add_instance(const instance& inst) {instances.push_back(inst);is_bvh_built = false;}

    int build_recursive(std::vector<scene_prim_ref>& prims, int start, int end) {
        int node_idx = (int)nodes.size();
        nodes.push_back(scene_bvh_node());

        aabb bounds;
        for (int i = start; i < end; ++i) bounds.expand(prims[i].bounds);
        nodes[node_idx].bounds = bounds;

        int count = end - start;
        if (count == 1) {
            nodes[node_idx].left_idx = LEAF_NODE;
            nodes[node_idx].right_idx = -1;
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


    void build_BVH() {
        nodes.clear();
        if (spheres.empty() && quads.empty() && triangles.empty() && instances.empty()) {
            is_bvh_built = true;
            return;
        }

        std::vector<scene_prim_ref> prims;
        for (int i = 0; i < (int)spheres.size(); ++i) {
            aabb b = spheres[i].get_aabb();
            prims.push_back({ b, PRIM_SPHERE, i });
        }
        for (int i = 0; i < (int)quads.size(); ++i) {
            aabb b = quads[i].get_aabb();
            prims.push_back({ b,PRIM_QUAD, i });
        }
        for (int i = 0; i < (int)triangles.size(); ++i) {
            aabb b = triangles[i].get_aabb();
            prims.push_back({ b,PRIM_TRIANGLE, i });
        }
        for (int i = 0; i < (int)instances.size(); ++i) {
            // 实例的包围盒需要变换
            aabb local_bounds = prototypes[instances[i].mesh_index].get_aabb();
            // 避免未构建BVH的mesh
            if (local_bounds.min.x() > local_bounds.max.x())continue;
            
            aabb world_bounds;
            // 重新求包围盒
            for (int k = 0; k < 8; ++k) {
                vec3 pt(k & 1 ? local_bounds.max.x() : local_bounds.min.x(),
                    k & 2 ? local_bounds.max.y() : local_bounds.min.y(),
                    k & 4 ? local_bounds.max.z() : local_bounds.min.z());
                world_bounds.expand(instances[i].transform.transform_point(pt));
            }
            prims.push_back({ world_bounds,PRIM_INSTANCE, i });
        }

        // 预分配节点空间
        nodes.reserve(prims.size() * 2 - 1);

        // 递归建树
        build_recursive(prims, 0, (int)prims.size());

        is_bvh_built = true;
    }

	// 暂时不做加速结构
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
                bool hit_this = false;

                switch (node.prim_type) {
                    case PRIM_SPHERE: {
                        hit_this = spheres[node.prim_index].hit(r, t_min, closest_so_far, temp_rec);
                        break;
                    }
                    case PRIM_QUAD: {
                        hit_this = quads[node.prim_index].hit(r, t_min, closest_so_far, temp_rec);
                        break;
                    }
                    case PRIM_TRIANGLE: {
                        hit_this = triangles[node.prim_index].hit(r, t_min, closest_so_far, temp_rec);
                        break;
                    }
                    case PRIM_INSTANCE: {
                        hit_this = instances[node.prim_index].hit(r, t_min, closest_so_far, temp_rec, prototypes);
                        break;
                    }
                }

                if (hit_this) {
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
};