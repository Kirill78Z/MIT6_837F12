#include "surf.h"
#include "extra.h"
using namespace std;

namespace
{

	// We're only implenting swept surfaces where the profile curve is
	// flat on the xy-plane.  This is a check function.
	static bool checkFlat(const Curve& profile)
	{
		for (unsigned i = 0; i < profile.size(); i++)
			if (profile[i].V[2] != 0.0 ||
				profile[i].T[2] != 0.0 ||
				profile[i].N[2] != 0.0)
				return false;

		return true;
	}
}

void addTriangles(Surface& surface, unsigned path_step, unsigned profile_step, unsigned profile_steps_number)
{
	Tup3u triangle;

	triangle[0] = path_step * profile_steps_number + profile_step;
	triangle[1] = (path_step - 1) * profile_steps_number + profile_step - 1;
	triangle[2] = (path_step - 1) * profile_steps_number + profile_step;
	surface.VF.push_back(triangle);

	triangle[1] = path_step * profile_steps_number + profile_step - 1;
	triangle[2] = (path_step - 1) * profile_steps_number + profile_step - 1;
	surface.VF.push_back(triangle);
}

Surface makeSurfRev(const Curve& profile, unsigned steps)
{
	Surface surface;

	if (!checkFlat(profile))
	{
		cerr << "surfRev profile curve must be flat on xy plane." << endl;
		exit(0);
	}

	for (unsigned surf_rotation_step = 0; surf_rotation_step <= steps; ++surf_rotation_step)
	{
		// step from 0 to 2pi
		float rotation = 2.0f * M_PI * float(surf_rotation_step) / steps;
		Matrix3f rotation_matrix = Matrix3f::rotation(Vector3f::UP, rotation);

		for (unsigned curve_pt_index = 0; curve_pt_index < profile.size(); ++curve_pt_index)
		{
			//vertex
			surface.VV.push_back(rotation_matrix * profile[curve_pt_index].V);

			//normal (assume normals will always point to the left of the direction of travel)
			//Vector3f normal = Vector3f::cross(profile[curve_pt_index].T, -Vector3f::FORWARD);

			//INTERESTING FACT: If a point is transformed by matrix M (INCLUDING NON-UNIFORM SCALING OR SHEAR),
			//the correct way to transform a normal vector is by using the inverse transpose of the upper-left 3x3 part of M.
			//SO IT IS MORE GENERAL WAY TRANSFORM NORMALS, BUT WE DON'T NEED IT HERE

			surface.VN.push_back(rotation_matrix * profile[curve_pt_index].N);

			if (surf_rotation_step > 0 && curve_pt_index > 0)
			{
				//triangles
				addTriangles(surface, surf_rotation_step, curve_pt_index, profile.size());
			}
		}

	}

	return surface;
}

Surface makeGenCyl(const Curve& profile, const Curve& sweep)
{
	Surface surface;

	if (!checkFlat(profile))
	{
		cerr << "genCyl profile curve must be flat on xy plane." << endl;
		exit(0);
	}

	for (unsigned sweep_step = 0; sweep_step < sweep.size(); ++sweep_step)
	{
		const CurvePoint& sweep_pt = sweep[sweep_step];
		Matrix4f position_transform = Matrix4f::identity();
		position_transform.setCol(0, Vector4f(sweep_pt.N, 0.0f));
		position_transform.setCol(1, Vector4f(sweep_pt.B, 0.0f));
		position_transform.setCol(2, Vector4f(sweep_pt.T, 0.0f));
		position_transform.setCol(3, Vector4f(sweep_pt.V, 1.0f));

		Matrix3f normal_transform = position_transform.getSubmatrix3x3(0,0);

		for (unsigned profile_step = 0; profile_step < profile.size(); ++profile_step)
		{
			//vertex
			Vector4f pos(profile[profile_step].V, 1.0f);
			pos = position_transform * pos;
			surface.VV.push_back(pos.xyz());

			//normal
			Vector3f normal = normal_transform * profile[profile_step].N;
			//normal.negate();//if profile is circle need to negate for proper render
			surface.VN.push_back(normal);

			if (sweep_step > 0 && profile_step > 0)
			{
				//triangles
				addTriangles(surface, sweep_step, profile_step, profile.size());
			}
		}
	}

	return surface;
}

void drawSurface(const Surface& surface, bool shaded)
{
	// Save current state of OpenGL
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	if (shaded)
	{
		// This will use the current material color and light
		// positions.  Just set these in drawScene();
		glEnable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// This tells openGL to *not* draw backwards-facing triangles.
		// This is more efficient, and in addition it will help you
		// make sure that your triangles are drawn in the right order.
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	else
	{
		glDisable(GL_LIGHTING);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glColor4f(0.4f, 0.4f, 0.4f, 1.f);
		glLineWidth(1);
	}

	glBegin(GL_TRIANGLES);
	for (unsigned i = 0; i < surface.VF.size(); i++)
	{
		glNormal(surface.VN[surface.VF[i][0]]);
		glVertex(surface.VV[surface.VF[i][0]]);
		glNormal(surface.VN[surface.VF[i][1]]);
		glVertex(surface.VV[surface.VF[i][1]]);
		glNormal(surface.VN[surface.VF[i][2]]);
		glVertex(surface.VV[surface.VF[i][2]]);
	}
	glEnd();

	glPopAttrib();
}

void drawNormals(const Surface& surface, float len)
{
	// Save current state of OpenGL
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glDisable(GL_LIGHTING);
	glColor4f(0, 1, 1, 1);
	glLineWidth(1);

	glBegin(GL_LINES);
	for (unsigned i = 0; i < surface.VV.size(); i++)
	{
		glVertex(surface.VV[i]);
		glVertex(surface.VV[i] + surface.VN[i] * len);
	}
	glEnd();

	glPopAttrib();
}

void outputObjFile(ostream& out, const Surface& surface)
{

	for (unsigned i = 0; i < surface.VV.size(); i++)
		out << "v  "
		<< surface.VV[i][0] << " "
		<< surface.VV[i][1] << " "
		<< surface.VV[i][2] << endl;

	for (unsigned i = 0; i < surface.VN.size(); i++)
		out << "vn "
		<< surface.VN[i][0] << " "
		<< surface.VN[i][1] << " "
		<< surface.VN[i][2] << endl;

	out << "vt  0 0 0" << endl;

	for (unsigned i = 0; i < surface.VF.size(); i++)
	{
		out << "f  ";
		for (unsigned j = 0; j < 3; j++)
		{
			unsigned a = surface.VF[i][j] + 1;
			out << a << "/" << "1" << "/" << a << " ";
		}
		out << endl;
	}
}
