#include "curve.h"
#include "extra.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
using namespace std;

namespace
{
	// Approximately equal to.  We don't want to use == because of
	// precision issues with floating point.
	inline bool approx(const Vector3f& lhs, const Vector3f& rhs)
	{
		const float eps = 1e-8f;
		return (lhs - rhs).absSquared() < eps;
	}


}

const Matrix4f BSplineBasis(
	1.0f / 6.0f, -3.0f / 6.0f, 3.0f / 6.0f, -1.0f / 6.0f,
	4.0f / 6.0f, 0.0f, -6.0f / 6.0f, 3.0f / 6.0f,
	1.0f / 6.0f, 3.0f / 6.0f, 3.0f / 6.0f, -3.0f / 6.0f,
	0.0f, 0.0f, 0.0f, 1.0f / 6.0f);

const Matrix4f BezierBasis(
	1.0f, -3.0f, 3.0f, -1.0f,
	0.0f, 3.0f, -6.0f, 3.0f,
	0.0f, 0.0f, 3.0f, -3.0f,
	0.0f, 0.0f, 0.0f, 1.0f
);

const Matrix4f BezierBasisInversed = BezierBasis.inverse();

const Matrix4f BSplineBasisByBezierBasisInversed = BSplineBasis * BezierBasisInversed;

Vector3f getAnyNormalTo(Vector3f vec)
{
	const Vector3f basisVectors[] = { Vector3f::UP ,Vector3f::RIGHT ,Vector3f::FORWARD };

	Vector3f crossProduct;
	int i = 0;
	do
	{
		crossProduct = Vector3f::cross(vec, basisVectors[i++]);
	} while (Vector3f::dot(crossProduct, crossProduct) < 1E-9f);

	return crossProduct.normalized();
}


Curve evalBezier(const vector< Vector3f >& P, unsigned steps)
{
	// Check
	if (P.size() < 4 || P.size() % 3 != 1)
	{
		cerr << "evalBezier must be called with 3n+1 control points." << endl;
		exit(0);
	}

	Curve result(steps + 1);

	for (unsigned i = 0; i <= steps; ++i)
	{
		float t = static_cast<float>(i) / steps;
		float one_minus_t = (1 - t);
		result[i].V =
			P[0] * one_minus_t * one_minus_t * one_minus_t
			+ P[1] * 3 * t * one_minus_t * one_minus_t
			+ P[2] * 3 * t * t * one_minus_t
			+ P[3] * t * t * t;

		result[i].T = (-3 * (P[0] * one_minus_t * one_minus_t + P[1] * (-3 * t * t + 4 * t - 1) + t * (3 * P[2] * t - 2 * P[2] - P[3] * t))).normalized();

		if (i == 0)
		{
			//first normal is taken arbitrary
			result[i].N = getAnyNormalTo(result[i].T);
		}
		else
		{
			//use previous binormal to get next normal
			result[i].N = Vector3f::cross(result[i - 1].B, result[i].T);
		}

		result[i].B = Vector3f::cross(result[i].T, result[i].N).normalized();
	}

	return result;
}

Curve evalBspline(const vector< Vector3f >& P, unsigned steps)
{
	// Check
	if (P.size() < 4)
	{
		cerr << "evalBspline must be called with 4 or more control points." << endl;
		exit(0);
	}

	static const Vector4f col0 = BSplineBasisByBezierBasisInversed.getCol(0);
	static const Vector4f col1 = BSplineBasisByBezierBasisInversed.getCol(1);
	static const Vector4f col2 = BSplineBasisByBezierBasisInversed.getCol(2);
	static const Vector4f col3 = BSplineBasisByBezierBasisInversed.getCol(3);

	Curve result;

	for (int i = 0; i < P.size() - 3; ++i)
	{
		vector<Vector3f> P_b_spl_seg = { P[i],P[i + 1], P[i + 2], P[i + 3] };
		vector<Vector3f> P_bezier =
		{
			Vector3f(
				P_b_spl_seg[0].x() * col0[0] + P_b_spl_seg[1].x() * col0[1] + P_b_spl_seg[2].x() * col0[2] + P_b_spl_seg[3].x() * col0[3],
				P_b_spl_seg[0].y() * col0[0] + P_b_spl_seg[1].y() * col0[1] + P_b_spl_seg[2].y() * col0[2] + P_b_spl_seg[3].y() * col0[3],
				P_b_spl_seg[0].z() * col0[0] + P_b_spl_seg[1].z() * col0[1] + P_b_spl_seg[2].z() * col0[2] + P_b_spl_seg[3].z() * col0[3]),
			Vector3f(
				P_b_spl_seg[0].x() * col1[0] + P_b_spl_seg[1].x() * col1[1] + P_b_spl_seg[2].x() * col1[2] + P_b_spl_seg[3].x() * col1[3],
				P_b_spl_seg[0].y() * col1[0] + P_b_spl_seg[1].y() * col1[1] + P_b_spl_seg[2].y() * col1[2] + P_b_spl_seg[3].y() * col1[3],
				P_b_spl_seg[0].z() * col1[0] + P_b_spl_seg[1].z() * col1[1] + P_b_spl_seg[2].z() * col1[2] + P_b_spl_seg[3].z() * col1[3]),
			Vector3f(
				P_b_spl_seg[0].x() * col2[0] + P_b_spl_seg[1].x() * col2[1] + P_b_spl_seg[2].x() * col2[2] + P_b_spl_seg[3].x() * col2[3],
				P_b_spl_seg[0].y() * col2[0] + P_b_spl_seg[1].y() * col2[1] + P_b_spl_seg[2].y() * col2[2] + P_b_spl_seg[3].y() * col2[3],
				P_b_spl_seg[0].z() * col2[0] + P_b_spl_seg[1].z() * col2[1] + P_b_spl_seg[2].z() * col2[2] + P_b_spl_seg[3].z() * col2[3]),
			Vector3f(
				P_b_spl_seg[0].x() * col3[0] + P_b_spl_seg[1].x() * col3[1] + P_b_spl_seg[2].x() * col3[2] + P_b_spl_seg[3].x() * col3[3],
				P_b_spl_seg[0].y() * col3[0] + P_b_spl_seg[1].y() * col3[1] + P_b_spl_seg[2].y() * col3[2] + P_b_spl_seg[3].y() * col3[3],
				P_b_spl_seg[0].z() * col3[0] + P_b_spl_seg[1].z() * col3[1] + P_b_spl_seg[2].z() * col3[2] + P_b_spl_seg[3].z() * col3[3]),
		};
		Curve bezier = evalBezier(P_bezier, steps);
		result.insert(result.end(), bezier.begin(), bezier.end());
	}

	return result;
}

Curve evalCircle(float radius, unsigned steps)
{
	// This is a sample function on how to properly initialize a Curve
	// (which is a vector< CurvePoint >).

	// Preallocate a curve with steps+1 CurvePoints
	Curve R(steps + 1);

	// Fill it in counterclockwise
	for (unsigned i = 0; i <= steps; ++i)
	{
		// step from 0 to 2pi
		float t = 2.0f * M_PI * float(i) / steps;

		// Initialize position
		// We're pivoting counterclockwise around the y-axis
		R[i].V = radius * Vector3f(cos(t), sin(t), 0);

		// Tangent vector is first derivative
		R[i].T = Vector3f(-sin(t), cos(t), 0);

		// Normal vector is second derivative
		R[i].N = Vector3f(-cos(t), -sin(t), 0);

		// Finally, binormal is facing up.
		R[i].B = Vector3f(0, 0, 1);
	}

	return R;
}

void drawCurve(const Curve& curve, float framesize)
{
	// Save current state of OpenGL
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	// Setup for line drawing
	glDisable(GL_LIGHTING);
	glColor4f(1, 1, 1, 1);
	glLineWidth(1);

	// Draw curve
	glBegin(GL_LINE_STRIP);
	for (unsigned i = 0; i < curve.size(); ++i)
	{
		glVertex(curve[i].V);
	}
	glEnd();

	glLineWidth(1);

	// Draw coordinate frames if framesize nonzero
	if (framesize != 0.0f)
	{
		Matrix4f M;

		for (unsigned i = 0; i < curve.size(); ++i)
		{
			M.setCol(0, Vector4f(curve[i].N, 0));
			M.setCol(1, Vector4f(curve[i].B, 0));
			M.setCol(2, Vector4f(curve[i].T, 0));
			M.setCol(3, Vector4f(curve[i].V, 1));

			glPushMatrix();
			glMultMatrixf(M);
			glScaled(framesize, framesize, framesize);
			glBegin(GL_LINES);
			glColor3f(1, 0, 0); glVertex3d(0, 0, 0); glVertex3d(1, 0, 0);
			glColor3f(0, 1, 0); glVertex3d(0, 0, 0); glVertex3d(0, 1, 0);
			glColor3f(0, 0, 1); glVertex3d(0, 0, 0); glVertex3d(0, 0, 1);
			glEnd();
			glPopMatrix();
		}
	}

	// Pop state
	glPopAttrib();
}

