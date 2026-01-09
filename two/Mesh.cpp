#include "Mesh.h"

#include <cassert>

using namespace std;

void Mesh::load(const char* filename)
{
	// 2.1.1. load() should populate bindVertices, currentVertices, and faces

	// Add your code here.
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open the file: " << filename << '\n';
		return;
	}

	static constexpr std::streamsize MAX_BUFFER_SIZE = 256;
	char buffer[MAX_BUFFER_SIZE];
	while (true)
	{
		file.getline(buffer, MAX_BUFFER_SIZE);
		stringstream ss(buffer);

		string s;
		ss >> s;
		if (s.empty()) {
			file.getline(buffer, MAX_BUFFER_SIZE);
			ss = stringstream(buffer);
			ss >> s;
			if (s.empty())
				break;
		}

		if (s == "v") {
			Vector3f v;
			ss >> v[0] >> v[1] >> v[2];
			bindVertices.push_back(v);
		}

		if (s == "f") {
			Tuple3u face;

			ss >> face[0] >> face[1] >> face[2];
			faces.push_back(face);
		}
	}

	// make a copy of the bind vertices as the current vertices
	currentVertices = bindVertices;
}

void Mesh::draw()
{
	// Since these meshes don't have normals
	// be sure to generate a normal per triangle.
	// Notice that since we have per-triangle normals
	// rather than the analytical normals from
	// assignment 1, the appearance is "faceted".
	for (auto face : faces)
	{
		auto vert0 = currentVertices[face[0] - 1];
		auto vert1 = currentVertices[face[1] - 1];
		auto vert2 = currentVertices[face[2] - 1];

		auto normal = Vector3f::cross((vert1 - vert0), (vert2 - vert0)).normalized();

		glBegin(GL_TRIANGLES);

		glNormal3d(normal.x(), normal.y(), normal.z());
		glVertex3d(vert0.x(), vert0.y(), vert0.z());

		glNormal3d(normal.x(), normal.y(), normal.z());
		glVertex3d(vert1.x(), vert1.y(), vert1.z());

		glNormal3d(normal.x(), normal.y(), normal.z());
		glVertex3d(vert2.x(), vert2.y(), vert2.z());
		glEnd();
	}

}

void Mesh::loadAttachments(const char* filename, int numJoints)
{
	// 2.2. Implement this method to load the per-vertex attachment weights
	// this method should update m_mesh.attachments
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open the file: " << filename << '\n';
		return;
	}

	std::string str;
	int attachmentNumber = 0;
	while (true) {
		attachments.emplace_back(numJoints);
		attachments[attachmentNumber][0] = 0.0; //root joint always have zero weight
		for (int i = 0; i < numJoints - 1; ++i)
		{
			if (!std::getline(file, str, ' '))
				return;

			attachments[attachmentNumber][i + 1] = std::stof(str);
		}

		std::getline(file, str);
		assert(str.empty()); //skip new line character in the end of the line

		++attachmentNumber;
	}
}
