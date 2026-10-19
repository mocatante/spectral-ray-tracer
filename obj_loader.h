#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include "scene.h"
#include "material.h"

// 通过路径加载obj
inline mesh load_obj(const std::string& filename, const material& mat = material::Diffuse(Color(0.8f, 0.8f, 0.8f))) {
    mesh m;
    std::vector<Point> vertices;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open OBJ file: " << filename << '\n';
        return m;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "v") {
            float x, y, z;
            if (iss >> x >> y >> z) {
                vertices.push_back(Point(x, y, z));
            }
        }
        else if (type == "f") {
            std::vector<int> face_indices;
            std::string token;

            while (iss >> token) {
                size_t pos = token.find('/');
                std::string idx_str = (pos == std::string::npos) ? token : token.substr(0, pos);

                if (!idx_str.empty()) {
                    int idx = std::stoi(idx_str) - 1;
                    face_indices.push_back(idx);
                }
            }

            if (face_indices.size() >= 3) {
                bool out_of_bounds = false;
                for (int idx : face_indices) {
                    if (idx < 0 || idx >= static_cast<int>(vertices.size())) {
                        out_of_bounds = true;
                        break;
                    }
                }
                if (out_of_bounds) continue;

                const Point& p0 = vertices[face_indices[0]];
                for (size_t i = 1; i + 1 < face_indices.size(); ++i) {
                    const Point& p1 = vertices[face_indices[i]];
                    const Point& p2 = vertices[face_indices[i + 1]];
                    m.add(triangle(p0, p1, p2, mat));
                }
            }
        }
        // 忽略 vt, vn, mtllib, usemtl
    }

    file.close();
    return m;
}