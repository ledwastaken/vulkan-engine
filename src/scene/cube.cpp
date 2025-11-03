#include "scene/cube.h"

namespace scene
{
  Cube::Cube(float size)
  {
    std::vector<Vertex> vertices = {
      // front
      {
          .position = { 1.0f * size, -1.0f * size, -1.0f * size },
          .normal = { 0.0f, 0.0f, -1.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, -1.0f * size, -1.0f * size },
          .normal = { 0.0f, 0.0f, -1.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, 1.0f * size, -1.0f * size },
          .normal = { 0.0f, 0.0f, -1.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, 1.0f * size, -1.0f * size },
          .normal = { 0.0f, 0.0f, -1.0f },
          .uv = { 0.0f, 0.0f },
      },

      // left
      {
          .position = { -1.0f * size, -1.0f * size, -1.0f * size },
          .normal = { -1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, -1.0f * size, 1.0f * size },
          .normal = { -1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, 1.0f * size, 1.0f * size },
          .normal = { -1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, 1.0f * size, -1.0f * size },
          .normal = { -1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },

      // right
      {
          .position = { 1.0f * size, -1.0f * size, 1.0f * size },
          .normal = { 1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, -1.0f * size, -1.0f * size },
          .normal = { 1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, 1.0f * size, -1.0f * size },
          .normal = { 1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, 1.0f * size, 1.0f * size },
          .normal = { 1.0f, 0.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },

      // back
      {
          .position = { -1.0f * size, -1.0f * size, 1.0f * size },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, -1.0f * size, 1.0f * size },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, 1.0f * size, 1.0f * size },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, 1.0f * size, 1.0f * size },
          .normal = { 0.0f, 0.0f, 1.0f },
          .uv = { 0.0f, 0.0f },
      },

      // top
      {
          .position = { -1.0f * size, 1.0f * size, 1.0f * size },
          .normal = { 0.0f, 1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, 1.0f * size, 1.0f * size },
          .normal = { 0.0f, 1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, 1.0f * size, -1.0f * size },
          .normal = { 0.0f, 1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, 1.0f * size, -1.0f * size },
          .normal = { 0.0f, 1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },

      // bottom
      {
          .position = { -1.0f * size, -1.0f * size, -1.0f * size },
          .normal = { 0.0f, -1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, -1.0f * size, -1.0f * size },
          .normal = { 0.0f, -1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { 1.0f * size, -1.0f * size, 1.0f * size },
          .normal = { 0.0f, -1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
      {
          .position = { -1.0f * size, -1.0f * size, 1.0f * size },
          .normal = { 0.0f, -1.0f, 0.0f },
          .uv = { 0.0f, 0.0f },
      },
    };

    std::vector<uint32_t> indices = {
      // front
      0,
      1,
      2,
      2,
      3,
      0,

      // left
      4,
      5,
      6,
      6,
      7,
      4,

      // right
      8,
      9,
      10,
      10,
      11,
      8,

      // back
      12,
      13,
      14,
      14,
      15,
      12,

      // top
      16,
      17,
      18,
      18,
      19,
      16,

      // bottom
      20,
      21,
      22,
      22,
      23,
      20,
    };

    load_mesh_data(vertices, indices);
  }
} // namespace scene