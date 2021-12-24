#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// This template is derived from
// https://github.com/vivkin/forsyth/blob/master/forsyth.h
// Based on Tom Forsyth's vertex cache optimizer

/*
  Copyright (C) 2008 Martin Storsjo
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

template <class VERTEX_INDEX_TYPE = uint32_t>
class VertexCacheOptimizer {
	// The size of these data types affect the memory usage
	typedef uint16_t FORSYTH_SCORE_TYPE;
	const int FORSYTH_SCORE_SCALING = 7281;

	typedef uint8_t FORSYTH_ADJACENCY_TYPE;
	const int FORSYTH_MAX_ADJACENCY = UINT8_MAX;

	typedef int8_t FORSYTH_CACHE_POS_TYPE;
	typedef int32_t FORSYTH_TRIANGLE_INDEX_TYPE;
	typedef int32_t FORSYTH_ARRAY_INDEX_TYPE;

	// The size of the precalculated tables
	static const int FORSYTH_CACHE_SCORE_TABLE_SIZE = 32;
	static const int FORSYTH_VALENCE_SCORE_TABLE_SIZE = 32;

	// Score function constants
	const real_t FORSYTH_CACHE_DECAY_POWER = 1.5;
	const real_t FORSYTH_LAST_TRI_SCORE = 0.75;
	const real_t FORSYTH_VALENCE_BOOST_SCALE = 2.0;
	const real_t FORSYTH_VALENCE_BOOST_POWER = 0.5;

	// Set these to adjust the performance and result quality
	static const int FORSYTH_VERTEX_CACHE_SIZE = 24;
	const int FORSYTH_CACHE_FUNCTION_LENGTH = 32;

	static_assert(FORSYTH_CACHE_SCORE_TABLE_SIZE >= FORSYTH_VERTEX_CACHE_SIZE, "Vertex score table too small");

	// Precalculated tables
	FORSYTH_SCORE_TYPE _forsyth_cache_position_score[FORSYTH_CACHE_SCORE_TABLE_SIZE];
	FORSYTH_SCORE_TYPE _forsyth_valence_score[FORSYTH_VALENCE_SCORE_TABLE_SIZE];

	int forsyth_is_added(const uint8_t *p_triangle_added, int p_x) const {
		return p_triangle_added[(p_x) >> 3] & (1 << (p_x & 7));
	}

	void forsyth_set_added(uint8_t *p_triangle_added, int p_x) const {
		p_triangle_added[(p_x) >> 3] |= (1 << (p_x & 7));
	}

	// Precalculate the tables
	void forsyth_init() {
		for (int i = 0; i < FORSYTH_CACHE_SCORE_TABLE_SIZE; i++) {
			float score = 0;
			if (i < 3) {
				// This vertex was used in the last triangle,
				// so it has a fixed score, which ever of the three
				// it's in. Otherwise, you can get very different
				// answers depending on whether you add
				// the triangle 1,2,3 or 3,1,2 - which is silly
				score = FORSYTH_LAST_TRI_SCORE;
			} else {
				// Points for being high in the cache.
				const float scaler = 1.0f / (FORSYTH_CACHE_FUNCTION_LENGTH - 3);
				score = 1.0f - (i - 3) * scaler;
				score = powf(score, FORSYTH_CACHE_DECAY_POWER);
			}
			_forsyth_cache_position_score[i] = (FORSYTH_SCORE_TYPE)(FORSYTH_SCORE_SCALING * score);
		}

		for (int i = 1; i < FORSYTH_VALENCE_SCORE_TABLE_SIZE; i++) {
			// Bonus points for having a low number of tris still to
			// use the vert, so we get rid of lone verts quickly
			float valenceBoost = powf(i, -FORSYTH_VALENCE_BOOST_POWER);
			float score = FORSYTH_VALENCE_BOOST_SCALE * valenceBoost;
			_forsyth_valence_score[i] = (FORSYTH_SCORE_TYPE)(FORSYTH_SCORE_SCALING * score);
		}
	}

	// Calculate the score for a vertex
	FORSYTH_SCORE_TYPE forsythFindVertexScore(int numActiveTris, int cachePosition) {
		if (numActiveTris == 0) {
			// No triangles need this vertex!
			return 0;
		}

		FORSYTH_SCORE_TYPE score = 0;
		if (cachePosition < 0) {
			// Vertex is not in LRU cache - no score
		} else {
			score = _forsyth_cache_position_score[cachePosition];
		}

		if (numActiveTris < FORSYTH_VALENCE_SCORE_TABLE_SIZE)
			score += _forsyth_valence_score[numActiveTris];
		return score;
	}

public:
	VertexCacheOptimizer() {
		forsyth_init();
	}

	// The main reordering function
	VERTEX_INDEX_TYPE *reorder_indices(VERTEX_INDEX_TYPE *outIndices, const VERTEX_INDEX_TYPE *indices, int nTriangles, int nVertices) {
		FORSYTH_ADJACENCY_TYPE *numActiveTris = (FORSYTH_ADJACENCY_TYPE *)malloc(sizeof(FORSYTH_ADJACENCY_TYPE) * nVertices);
		memset(numActiveTris, 0, sizeof(FORSYTH_ADJACENCY_TYPE) * nVertices);

		// First scan over the vertex data, count the total number of
		// occurrances of each vertex
		for (int i = 0; i < 3 * nTriangles; i++) {
			if (numActiveTris[indices[i]] == FORSYTH_MAX_ADJACENCY) {
				// Unsupported mesh,
				// vertex shared by too many triangles
				free(numActiveTris);
				return NULL;
			}
			numActiveTris[indices[i]]++;
		}

		// Allocate the rest of the arrays
		FORSYTH_ARRAY_INDEX_TYPE *offsets = (FORSYTH_ARRAY_INDEX_TYPE *)malloc(sizeof(FORSYTH_ARRAY_INDEX_TYPE) * nVertices);
		FORSYTH_SCORE_TYPE *lastScore = (FORSYTH_SCORE_TYPE *)malloc(sizeof(FORSYTH_SCORE_TYPE) * nVertices);
		FORSYTH_CACHE_POS_TYPE *cacheTag = (FORSYTH_CACHE_POS_TYPE *)malloc(sizeof(FORSYTH_CACHE_POS_TYPE) * nVertices);

		uint8_t *triangleAdded = (uint8_t *)malloc((nTriangles + 7) / 8);
		FORSYTH_SCORE_TYPE *triangleScore = (FORSYTH_SCORE_TYPE *)malloc(sizeof(FORSYTH_SCORE_TYPE) * nTriangles);
		FORSYTH_TRIANGLE_INDEX_TYPE *triangleIndices = (FORSYTH_TRIANGLE_INDEX_TYPE *)malloc(sizeof(FORSYTH_TRIANGLE_INDEX_TYPE) * 3 * nTriangles);
		memset(triangleAdded, 0, sizeof(uint8_t) * ((nTriangles + 7) / 8));
		memset(triangleScore, 0, sizeof(FORSYTH_SCORE_TYPE) * nTriangles);
		memset(triangleIndices, 0, sizeof(FORSYTH_TRIANGLE_INDEX_TYPE) * 3 * nTriangles);

		// Count the triangle array offset for each vertex,
		// initialize the rest of the data.
		int sum = 0;
		for (int i = 0; i < nVertices; i++) {
			offsets[i] = sum;
			sum += numActiveTris[i];
			numActiveTris[i] = 0;
			cacheTag[i] = -1;
		}

		// Fill the vertex data structures with indices to the triangles
		// using each vertex
		for (int i = 0; i < nTriangles; i++) {
			for (int j = 0; j < 3; j++) {
				int v = indices[3 * i + j];
				triangleIndices[offsets[v] + numActiveTris[v]] = i;
				numActiveTris[v]++;
			}
		}

		// Initialize the score for all vertices
		for (int i = 0; i < nVertices; i++) {
			lastScore[i] = forsythFindVertexScore(numActiveTris[i], cacheTag[i]);
			for (int j = 0; j < numActiveTris[i]; j++)
				triangleScore[triangleIndices[offsets[i] + j]] += lastScore[i];
		}

		// Find the best triangle
		int bestTriangle = -1;
		int bestScore = -1;

		for (int i = 0; i < nTriangles; i++) {
			if (triangleScore[i] > bestScore) {
				bestScore = triangleScore[i];
				bestTriangle = i;
			}
		}

		// Allocate the output array
		FORSYTH_TRIANGLE_INDEX_TYPE *outTriangles = (FORSYTH_TRIANGLE_INDEX_TYPE *)malloc(sizeof(FORSYTH_TRIANGLE_INDEX_TYPE) * nTriangles);
		int outPos = 0;

		// Initialize the cache
		int cache[FORSYTH_VERTEX_CACHE_SIZE + 3];
		for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE + 3; i++)
			cache[i] = -1;

		int scanPos = 0;

		// Output the currently best triangle, as long as there
		// are triangles left to output
		while (bestTriangle >= 0) {
			// Mark the triangle as added
			forsyth_set_added(triangleAdded, bestTriangle);
			// Output this triangle
			outTriangles[outPos++] = bestTriangle;
			for (int i = 0; i < 3; i++) {
				// Update this vertex
				int v = indices[3 * bestTriangle + i];

				// Check the current cache position, if it
				// is in the cache
				int endpos = cacheTag[v];
				if (endpos < 0)
					endpos = FORSYTH_VERTEX_CACHE_SIZE + i;
				if (endpos > i) {
					// Move all cache entries from the previous position
					// in the cache to the new target position (i) one
					// step backwards
					for (int j = endpos; j > i; j--) {
						cache[j] = cache[j - 1];
						// If this cache slot contains a real
						// vertex, update its cache tag
						if (cache[j] >= 0)
							cacheTag[cache[j]]++;
					}
					// Insert the current vertex into its new target
					// slot
					cache[i] = v;
					cacheTag[v] = i;
				}

				// Find the current triangle in the list of active
				// triangles and remove it (moving the last
				// triangle in the list to the slot of this triangle).
				for (int j = 0; j < numActiveTris[v]; j++) {
					if (triangleIndices[offsets[v] + j] == bestTriangle) {
						triangleIndices[offsets[v] + j] = triangleIndices[offsets[v] + numActiveTris[v] - 1];
						break;
					}
				}
				// Shorten the list
				numActiveTris[v]--;
			}
			// Update the scores of all triangles in the cache
			for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE + 3; i++) {
				int v = cache[i];
				if (v < 0)
					break;
				// This vertex has been pushed outside of the
				// actual cache
				if (i >= FORSYTH_VERTEX_CACHE_SIZE) {
					cacheTag[v] = -1;
					cache[i] = -1;
				}
				FORSYTH_SCORE_TYPE newScore = forsythFindVertexScore(numActiveTris[v], cacheTag[v]);
				FORSYTH_SCORE_TYPE diff = newScore - lastScore[v];
				for (int j = 0; j < numActiveTris[v]; j++)
					triangleScore[triangleIndices[offsets[v] + j]] += diff;
				lastScore[v] = newScore;
			}
			// Find the best triangle referenced by vertices in the cache
			bestTriangle = -1;
			bestScore = -1;
			for (int i = 0; i < FORSYTH_VERTEX_CACHE_SIZE; i++) {
				if (cache[i] < 0)
					break;
				int v = cache[i];
				for (int j = 0; j < numActiveTris[v]; j++) {
					int t = triangleIndices[offsets[v] + j];
					if (triangleScore[t] > bestScore) {
						bestTriangle = t;
						bestScore = triangleScore[t];
					}
				}
			}
			// If no active triangle was found at all, continue
			// scanning the whole list of triangles
			if (bestTriangle < 0) {
				for (; scanPos < nTriangles; scanPos++) {
					if (!forsyth_is_added(triangleAdded, scanPos)) {
						bestTriangle = scanPos;
						break;
					}
				}
			}
		}

		// Convert the triangle index array into a full triangle list
		outPos = 0;
		for (int i = 0; i < nTriangles; i++) {
			int t = outTriangles[i];
			for (int j = 0; j < 3; j++) {
				int v = indices[3 * t + j];
				outIndices[outPos++] = v;
			}
		}

		// Clean up
		free(triangleIndices);
		free(offsets);
		free(lastScore);
		free(numActiveTris);
		free(cacheTag);
		free(triangleAdded);
		free(triangleScore);
		free(outTriangles);

		return outIndices;
	}
};
