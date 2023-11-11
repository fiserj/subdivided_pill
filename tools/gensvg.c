// gensvg.c - One-off utility to generate SVG image to illustrate the pill
//            subdivision scheme.

#define _CRT_SECURE_NO_WARNINGS

#include <assert.h> // assert
#include <stdint.h> // uint*_t
#include <stdio.h>  // f*, stderr
#include <stdlib.h> // atoi, calloc, free

#define SUBIDIVIDED_PILL_IMPLEMENTATION
#include "../subdivided_pill.h" // sp_*

typedef struct {
  float x;
  float y;
} Vertex;

typedef struct {
  uint16_t i;
  uint16_t j;
  uint16_t k;
} Triangle;

int main(int _argc, char** _argv) {
  if (_argc != 3) {
    fprintf(stderr, "USAGE:\n  gensvg subdivision_count output_svg\n");
    return 1;
  }

  const int max_subdivision_count = 7;
  const int subdivision_count = atoi(_argv[1]);
  if (subdivision_count < 0 || subdivision_count > max_subdivision_count) {
    fprintf(
      stderr,
      "Subdivision count must be >= 0 and <= %i (was %i).\n",
      max_subdivision_count,
      subdivision_count);
    return 2;
  }

  const char* output_svg = _argv[2];
  FILE* svg = fopen(output_svg, "wb");
  if (!svg) {
    fprintf(stderr, "Failed to open file \"%s\".\n", output_svg);
    return 3;
  }

  const int vertex_count = sp_get_vertex_count(subdivision_count);
  Vertex* vertices = (Vertex*)calloc(1, vertex_count * sizeof(Vertex));
  assert(vertices);

  const int triangle_count = sp_get_triangle_count(subdivision_count);
  Triangle* triangles = (Triangle*)calloc(1, triangle_count * sizeof(Triangle));
  assert(triangles);

  sp_create_geometry(subdivision_count, &vertices[0].x, &triangles[0].i);

  fprintf(
    svg,
    "<svg version=\"1.1\" "
    "viewBox=\"-0.05 -0.55 %.2f 1.1\" "
    "xmlns=\"http://www.w3.org/2000/svg\">",
    subdivision_count * 2.125f + 1.1f);

  // https://sashamaps.net/docs/resources/20-colors/
  const uint32_t colors[] = {
    0x13594e,
    0x1d8676,
    0x26b29d,
    0x30dfc4,
    0x59e5d0,
    0x83ecdc,
    0xacf2e7,
    0xd6f9f3,
  };
  assert(sizeof(colors) / sizeof(colors[0]) > max_subdivision_count);

  for (int i = 0; i <= subdivision_count; i++) {
    const int first = i == 0 ? 0 : sp_get_triangle_count(i - 1);
    const int last = sp_get_triangle_count(i);

    fprintf(
      svg,
      "\n  <!-- Subdivision level #%i (%s%i triangles) -->\n",
      i,
      i > 0 ? "+" : "",
      last - first);

    fprintf(
      svg,
      "  <g id=\"level-%i\" fill=\"rgb(%3i,%3i,%3i)\" "
      "transform=\"translate(%.3f 0)\" stroke-width=\"0.0025\" "
      "stroke=\"black\" stroke-linejoin=\"round\" stroke-linecap=\"round\">\n",
      i,
      (colors[i] & 0xff0000) >> 16,
      (colors[i] & 0x00ff00) >> 8,
      (colors[i] & 0x0000ff),
      i * 2.125f - (i > 0 ? 0.5f : 0.0f));

    if (i > 0) {
      fprintf(
        svg,
        "    <use href=\"#level-%i\" transform=\"translate(-%.3f 0)\" />\n",
        i - 1,
        (i - 1) * 2.125f - (i > 0 ? 0.5f : 0.0f));
    }

    for (int j = first; j < last; j++) {
      const Vertex v0 = vertices[triangles[j].i];
      const Vertex v1 = vertices[triangles[j].j];
      const Vertex v2 = vertices[triangles[j].k];

      // clang-format off
      fprintf(
        svg,
        "    <path d=\"M % .4f % .4f L % .4f % .4f L % .4f % .4f Z\" />\n",
        v0.x, v0.y,
        v1.x, v1.y,
        v2.x, v2.y);
      // clang-format on
    }

    fprintf(svg, "  </g>\n");
  }

  fprintf(svg, "</svg>\n");
  fclose(svg);

  return 0;
}
