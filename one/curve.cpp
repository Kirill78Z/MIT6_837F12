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


void populateBezierCurveSegment(Curve& result, const vector<Vector3f>& P, int start_index, unsigned steps)
{
	for (unsigned curve_pt_index = start_index, i = 0; curve_pt_index <= start_index + steps; ++curve_pt_index, ++i)
	{
		float t = static_cast<float>(i) / steps;
		float one_minus_t = (1 - t);
		result[curve_pt_index].V =
			P[0] * one_minus_t * one_minus_t * one_minus_t
			+ P[1] * 3 * t * one_minus_t * one_minus_t
			+ P[2] * 3 * t * t * one_minus_t
			+ P[3] * t * t * t;

		result[curve_pt_index].T = (-3 * (P[0] * one_minus_t * one_minus_t + P[1] * (-3 * t * t + 4 * t - 1) + t * (3 * P[2] * t - 2 * P[2] - P[3] * t))).normalized();

		if (curve_pt_index == 0)
		{
			//first normal is taken arbitrary
			result[curve_pt_index].N = getAnyNormalTo(result[curve_pt_index].T);
		}
		else
		{
			//use previous binormal to get next normal
			result[curve_pt_index].N = Vector3f::cross(result[curve_pt_index - 1].B, result[curve_pt_index].T);
		}

		result[curve_pt_index].B = Vector3f::cross(result[curve_pt_index].T, result[curve_pt_index].N).normalized();
	}
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

	populateBezierCurveSegment(result, P, 0, steps);

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

	auto cubic_segment_number = P.size() - 3;
	Curve result(steps * cubic_segment_number + 1);

	for (size_t i = 0; i < cubic_segment_number; ++i)
	{
		vector<Vector3f> P_b_spl_seg = { P[i],P[i + 1], P[i + 2], P[i + 3] };
		Matrix4f P_b_spl_seg_matrix(
			P_b_spl_seg[0].x(), P_b_spl_seg[1].x(), P_b_spl_seg[2].x(), P_b_spl_seg[3].x(),
			P_b_spl_seg[0].y(), P_b_spl_seg[1].y(), P_b_spl_seg[2].y(), P_b_spl_seg[3].y(),
			P_b_spl_seg[0].z(), P_b_spl_seg[1].z(), P_b_spl_seg[2].z(), P_b_spl_seg[3].z(),
			0.0f, 0.0f, 0.0f, 0.0f);

		Matrix4f P_bezier_matrix = P_b_spl_seg_matrix * BSplineBasisByBezierBasisInversed;
		vector<Vector3f> P_bezier =
		{
			Vector3f(P_bezier_matrix(0,0),P_bezier_matrix(1,0),P_bezier_matrix(2,0)),
			Vector3f(P_bezier_matrix(0,1),P_bezier_matrix(1,1),P_bezier_matrix(2,1)),
			Vector3f(P_bezier_matrix(0,2),P_bezier_matrix(1,2),P_bezier_matrix(2,2)),
			Vector3f(P_bezier_matrix(0,3),P_bezier_matrix(1,3),P_bezier_matrix(2,3)),
		};

		populateBezierCurveSegment(result, P_bezier, i * steps, steps);
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

