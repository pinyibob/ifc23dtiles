#include "BintreeSplitor.h"


namespace XBSJ {
	struct SplitSort {
		SplitVector s;
		double sort = 0;
	};

	bool compSplitSort(SplitSort & t1, SplitSort &  t2) {
		return  t1.sort < t2.sort;
	}

	bool compSplitScalar(SplitScalar & t1, SplitScalar &  t2) {
		return  t1.value >  t2.value;
	}


	double biasValue(list<SplitSort> &input, size_t idx) {
		int cur = -1;
		double left = 0;
		double right = 0;
		for (auto& s : input) {
			cur++;
			if (cur <= idx)
				left += s.s.value;
			else
				right += s.s.value;
		}
		return fabs(left - right);
	}


	bool BintreeSplitor::split(list<SplitScalar> &input, list<SplitScalar> &c0, list<SplitScalar> &c1) {
		if (input.size() < 2)
			return false;
		if (input.size() == 2)
		{
			c0.push_back(*input.begin());
			c1.push_back(*(++input.begin()));
			return true;
		}

		//这个排序还是有意义的，否则份块结果将受限于输入顺序
		input.sort(compSplitScalar);
		double sum0 = 0;
		double sum1 = 0;
		for (auto s : input) {
			double v = s.value;
			//保证sum0 sum1的绝对值差更小
			if (fabs(v + sum0 - sum1) < fabs(v + sum1 - sum0)) {
				c0.push_back(s);
				sum0 += v;
			}
			else {
				c1.push_back(s);
				sum1 += v;
			}
		}

		return true;
	}
	bool BintreeSplitor::split(list<SplitVector> &input, list<SplitVector> &c0, list<SplitVector> &c1) {
		if (input.size() < 2)
			return false;
		if (input.size() == 2)
		{
			c0.push_back(*input.begin());
			c1.push_back(*(++input.begin()));
			return true;
		}

		//空间分割 理论上应该在x，y，z三个轴上寻找最佳分割，保证二分数据量差值最小
		list<SplitVector> x0, x1;
		list<SplitVector> y0, y1;
		list<SplitVector> z0, z1;
		double xbias, ybias, zbias;

		split(input, 0, x0, x1, xbias);
		split(input, 1, y0, y1, ybias);
		split(input, 2, z0, z1, zbias);

		//三者都分割失败，那么返回false
		if (xbias == FLT_MAX && ybias == FLT_MAX && zbias == FLT_MAX)
			return false;

		//x数据量偏移最小
		if (xbias <= ybias && xbias <= zbias) {
			c0 = move(x0); c1 = move(x1);
			return true;
		}
		//y数据量偏移最小
		if (ybias <= xbias && ybias <= zbias) {
			c0 = move(y0); c1 = move(y1);
			return true;
		}
		//返回z分割
		c0 = move(z0); c1 = move(z1);
		return true;
	}

 
	bool BintreeSplitor::split(list<SplitVector> &input, int axis, list<SplitVector> &c0, list<SplitVector> &c1, double  & bias) {

		bias = FLT_MAX;

		//构造排序
		list<SplitSort> sorts;
		for (auto s : input) {
			SplitSort ss;
			ss.s = s;

			if(axis == 0)
				ss.sort = s.pos.x();
			else if (axis == 1)
				ss.sort = s.pos.y();
			else
				ss.sort = s.pos.z();

			sorts.push_back(ss);
		}
		sorts.sort(compSplitSort);

		//在list中寻找一个合适的分界值，保证左右两侧的误差最小
		
		int minIdx = -1;

		for (size_t i = 0; i < sorts.size(); i++) {
			double b = biasValue(sorts, i);

			if (b < bias) {
				bias = b;
				minIdx = i;
			}
		}

		if (minIdx < 0)
			return false;

		//从这个位置对数据进行分割
		int i = -1;
		for (auto s: sorts) {
			i++;

			if (i <= minIdx) {
				c0.push_back(s.s);
			}
			else {
				c1.push_back(s.s);
			}
		}

		return true;

	}
	
}