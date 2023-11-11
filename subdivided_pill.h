// subdivided_pill.h - Simple 2D "pill shape" geometry generator with a topology
//                     suitable for instanced rendering, and continuous addition
//                     of geometric detail
//
// Project URL: https://github.com/fiserj/subdivided_pill
//
// Do this:
//   #define SUBIDIVIDED_PILL_IMPLEMENTATION
// before you include this file in *one* C or C++ file to create the
// implementation.

#ifndef SUBIDIVIDED_PILL_H
#define SUBIDIVIDED_PILL_H

#include <stdint.h> // int16_t, uint16_t

#ifdef __cplusplus
extern "C" {
#endif

// 4, 6, 10, 18, 34, 66, 130, ...
int sp_get_vertex_count(int _subdivision_count);

// 2, 4, 8, 16, 32, 64, 128, 256, 512, ...
int sp_get_triangle_count(int _subdivision_count);

// Returns minimum subdivision count for an artifact-free appearance.
int sp_get_optimum_subdivision_count(float _on_screen_radius, float _error_tolerance);

// Constructs the "canonical" pill vertex and index buffers.
void sp_create_geometry(int _subdivision_count, float* _out_vertices, uint16_t* _out_triangles);

// Reduces the vertex data footprint. See README for more information.
void sp_pack_vertices(int _vertex_count, const float* _vertices, int16_t* _out_packed_vertices);

#ifdef __cplusplus
}
#endif

#endif // SUBIDIVIDED_PILL_H

#ifdef SUBIDIVIDED_PILL_IMPLEMENTATION

#include <math.h> // acosf, ceilf, fmaxf, fminf, logf, powf, roundf, sqrtf

#ifndef SP_ASSERT
#  include <assert.h> // assert
#  define SP_ASSERT(_condition) assert(_condition)
#endif

typedef struct {
  int16_t x;
  int16_t y;
} sp__PackedVertex;

typedef struct {
  float x;
  float y;
} sp__Vertex;

typedef struct {
  uint16_t i[3];
} sp__Triangle;

static int16_t sp__to_snorm_16(float _value) {
  return (int16_t)roundf(fminf(fmaxf(_value, -1.0f), 1.0f) * 32767.0f);
}

// http://web.archive.org/web/20090216130745/https://www.humus.name/index.php?page=Comments&ID=228
static sp__Vertex sp__subdivide_chord(sp__Vertex _v0, sp__Vertex _v1) {
  const float mx = (_v0.x + _v1.x) * 0.5f;
  const float my = (_v0.y + _v1.y) * 0.5f;
  const float ms = 0.5f / sqrtf(mx * mx + my * my);

  const float x = ms * mx;
  const float y = ms * my;

  return (sp__Vertex){x, y};
}

void sp__create_geometry(int _subdivision_count, sp__Vertex* _out_vertices, sp__Triangle* _out_triangles) {
  SP_ASSERT(_subdivision_count >= 0);
  SP_ASSERT(_out_vertices);
  SP_ASSERT(_out_triangles);

  // Initial quad.
  _out_vertices[0] = (sp__Vertex){0.0f, -0.5f};
  _out_vertices[1] = (sp__Vertex){0.0f, +0.5f};
  _out_vertices[2] = (sp__Vertex){1.0f, +0.5f};
  _out_vertices[3] = (sp__Vertex){1.0f, -0.5f};

  _out_triangles[0] = (sp__Triangle){0, 1, 2};
  _out_triangles[1] = (sp__Triangle){2, 3, 0};

  if (_subdivision_count == 0) {
    return;
  }

  // End half-"circles" (triangles).
  _out_vertices[4] = (sp__Vertex){-0.5f, 0.0f};
  _out_vertices[5] = (sp__Vertex){+1.5f, 0.0f};

  _out_triangles[2] = (sp__Triangle){0, 4, 1};
  _out_triangles[3] = (sp__Triangle){2, 5, 3};

  if (_subdivision_count == 1) {
    return;
  }

  uint16_t next_vertex_index = 6;
  int next_triangle_index = 4;

  sp__Triangle* prev_triangles = _out_triangles + 2;
  sp__Triangle* next_triangles = _out_triangles + next_triangle_index;

  for (int subdivision = 1, triangle_count = 1;; subdivision++) {
    // Left half-circle (do the actual chord subdivision).
    for (int triangle = 0; triangle < triangle_count; triangle++) {
      for (int edge = 0; edge < 2; edge++) {
        const uint16_t i0 = prev_triangles[triangle].i[edge + 0];
        const uint16_t i1 = prev_triangles[triangle].i[edge + 1];

        const int triangle_index = triangle * 2 + edge;
        next_triangles[triangle_index] = (sp__Triangle){i0, next_vertex_index, i1};

        const sp__Vertex next_vertex = sp__subdivide_chord(_out_vertices[i0], _out_vertices[i1]);
        _out_vertices[next_vertex_index] = next_vertex;

        next_vertex_index++;
      } // edge
    }   // triangle

    // Right half-circle (simply copy and flip).
    for (int triangle = 0; triangle < triangle_count; triangle++) {
      for (int edge = 0; edge < 2; edge++) {
        const uint16_t i0 = prev_triangles[triangle_count + triangle].i[edge + 0];
        const uint16_t i1 = prev_triangles[triangle_count + triangle].i[edge + 1];

        const int triangle_index = (triangle_count + triangle) * 2 + edge;
        next_triangles[triangle_index] = (sp__Triangle){i0, next_vertex_index, i1};

        const sp__Vertex left_half_vertex = _out_vertices[next_vertex_index - triangle_count * 2];
        _out_vertices[next_vertex_index] = (sp__Vertex){-left_half_vertex.x + 1.0f, -left_half_vertex.y};

        next_vertex_index++;
      } // edge
    }   // triangle

    if (subdivision + 1 == _subdivision_count) {
      break;
    }

    next_triangle_index *= 2;
    triangle_count *= 2;

    prev_triangles = next_triangles;
    next_triangles = _out_triangles + next_triangle_index;
  } // subdivision
}

static void sp__pack_vertices(int _vertex_count, const sp__Vertex* _vertices, sp__PackedVertex* _out_packed_vertices) {
  SP_ASSERT(_vertex_count >= 0);
  SP_ASSERT(_vertices);
  SP_ASSERT(_out_packed_vertices);

  for (int i = 0; i < _vertex_count; i++) {
    // https://wwwtyro.net/2019/11/18/instanced-lines.html#special-case-round-caps-and-joins
    // Instead of Z value being either 0 or 1, we use -0.5 or 0.5 (see below).
    float x = _vertices[i].x;
    float z = -0.5f;

    if (x >= 1.0f) {
      x -= 1.0f;
      z = 0.5f;
    }

    // X and Y are now both in [-0.5, 0.5] range but to save space, we shift the
    // X values to ranges [-1.0, -0.5] and [0.5, 1.0], and let the sign bit
    // determine which side of the line the point belongs to.
    const int16_t xn = sp__to_snorm_16(x + z);
    const int16_t yn = sp__to_snorm_16(_vertices[i].y);
    SP_ASSERT(xn != 0);

    _out_packed_vertices[i] = (sp__PackedVertex){xn, yn};
  }
}

int sp_get_vertex_count(int _subdivision_count) {
  SP_ASSERT(_subdivision_count >= 0);
  return 2 + 2 * (int)powf(2.0f, (float)_subdivision_count);
}

int sp_get_triangle_count(int _subdivision_count) {
  SP_ASSERT(_subdivision_count >= 0);
  return (int)powf(2.0f, (float)(_subdivision_count + 1));
}

int sp_get_optimum_subdivision_count(float _on_screen_radius, float _error_tolerance) {
  const float vertex_count = 3.14159265358979323846f / acosf(1.0f - _error_tolerance / _on_screen_radius);

  // 3 * 2^subdivision_count >= vertex_count
  const float subdivision_count = logf(vertex_count / 3.0f) / logf(2.0);

  return (int)ceilf(subdivision_count);
}

void sp_create_geometry(int _subdivision_count, float* _out_vertices, uint16_t* _out_triangles) {
  sp__create_geometry(_subdivision_count, (sp__Vertex*)_out_vertices, (sp__Triangle*)_out_triangles);
}

void sp_pack_vertices(int _vertex_count, const float* _vertices, int16_t* _out_packed_vertices) {
  sp__pack_vertices(_vertex_count, (const sp__Vertex*)_vertices, (sp__PackedVertex*)_out_packed_vertices);
}

#endif // SUBIDIVIDED_PILL_IMPLEMENTATION
