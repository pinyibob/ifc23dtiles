#pragma once

#include "stdafx.h"

namespace XBSJ {


	template <typename T>
	struct Rect {
		inline Rect() {}
		inline Rect(T x_, T y_, T w_, T h_) : x(x_), y(y_), w(w_), h(h_) {}
		T x = 0, y = 0;
		T w = 0, h = 0;
		T originalW = 0, originalH = 0;

		template <typename Number>
		inline Rect operator *(Number value) const {
			return Rect(x * value, y * value, w * value, h * value);
		}

		template <typename R>
		inline bool operator==(const R& r) const {
			return x == r.x && y == r.y && w == r.w && h == r.h;
		}

		inline bool hasArea() const { return w != 0 && h != 0; }
	};

	template <typename T>
	class BinaryTreePack
	{
	public:
		BinaryTreePack(T width  =1024, T height = 1024) : free(1, Rect<uint16_t>{ 0, 0, width, height })
		{

		}
		Rect<T> allocate(T width, T height) {
			// 寻找最小的未占用的矩形空间
			auto smallest = free.end();
			for (auto it = free.begin(); it != free.end(); ++it) {
				const Rect<T>& ref = *it;
				if (width <= ref.w && height <= ref.h) {
					if (smallest == free.end()) {
						smallest = it;
					}
					else {
						const Rect<T>& rect = *smallest;
						if (ref.y <= rect.y && ref.x <= rect.x) {
							smallest = it;
						}
						// Our current "smallest" rect is already closer to 0/0.
					}
				}
				else {
					// The rect in the free list is not big enough.
				}
			}
			if (smallest == free.end()) {
				// There's no space left for this char.
				return Rect<uint16_t>{ 0, 0, 0, 0 };
			}
			else {
				Rect<T> rect = *smallest;
				free.erase(smallest);
				// Shorter/Longer Axis Split Rule (SAS)
				// http://clb.demon.fi/files/RectangleBinPack.pdf p. 15
				// Ignore the dimension of R and just split long the shorter dimension
				// See Also: http://www.cs.princeton.edu/~chazelle/pubs/blbinpacking.pdf
				if (rect.w < rect.h) {
					// split horizontally
					// +--+---+
					// |__|___|  <-- b1
					// +------+  <-- b2
					if (rect.w > width) free.emplace_back(rect.x + width, rect.y, rect.w - width, height);
					if (rect.h > height) free.emplace_back(rect.x, rect.y + height, rect.w, rect.h - height);
				}
				else {
					// split vertically
					// +--+---+
					// |__|   | <-- b1
					// +--|---+ <-- b2
					if (rect.w > width) free.emplace_back(rect.x + width, rect.y, rect.w - width, rect.h);
					if (rect.h > height) free.emplace_back(rect.x, rect.y + height, width, rect.h - height);
				}

				return Rect<uint16_t>{ rect.x, rect.y, width, height };
			}
		}

		void release(Rect<T> rect) {
			// Simple algorithm to recursively merge the newly released cell with its
			// neighbor. This doesn't merge more than two cells at a time, and fails
			// for complicated merges.
			for (auto it = free.begin(); it != free.end(); ++it) {
				Rect<T> ref = *it;
				if (ref.y == rect.y && ref.h == rect.h && ref.x + ref.w == rect.x) {
					ref.w += rect.w;
				}
				else if (ref.x == rect.x && ref.w == rect.w && ref.y + ref.h == rect.y) {
					ref.h += rect.h;
				}
				else if (rect.y == ref.y && rect.h == ref.h && rect.x + rect.w == ref.x) {
					ref.x = rect.x;
					ref.w += rect.w;
				}
				else if (rect.x == ref.x && rect.w == ref.w && rect.y + rect.h == ref.y) {
					ref.y = rect.y;
					ref.h += rect.h;
				}
				else {
					continue;
				}
				free.erase(it);
				release(ref);
				return;
			}

			free.emplace_back(rect);
		};

	private:
		std::list<Rect<T>> free;
	};
	typedef BinaryTreePack<uint16_t>  BinaryTreePack16;
}

