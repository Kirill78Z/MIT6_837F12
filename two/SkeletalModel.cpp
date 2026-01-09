#include "SkeletalModel.h"

#include <cassert>
#include <FL/Fl.H>

using namespace std;

void SkeletalModel::load(const char* skeletonFile, const char* meshFile, const char* attachmentsFile)
{
	loadSkeleton(skeletonFile);

	m_mesh.load(meshFile);
	m_mesh.loadAttachments(attachmentsFile, m_joints.size());

	computeBindWorldToJointTransforms();
	updateCurrentJointToWorldTransforms();
}

void SkeletalModel::draw(Matrix4f cameraMatrix, bool skeletonVisible)
{
	// draw() gets called whenever a redraw is required
	// (after an update() occurs, when the camera moves, the window is resized, etc)

	m_matrixStack.clear();
	m_matrixStack.push(cameraMatrix);

	if (skeletonVisible)
	{
		drawJoints();

		drawSkeleton();
	}
	else
	{
		// Clear out any weird matrix we may have been using for drawing the bones and revert to the camera matrix.
		glLoadMatrixf(m_matrixStack.top());

		// Tell the mesh to draw itself.
		m_mesh.draw();
	}
}

void SkeletalModel::loadSkeleton(const char* filename)
{
	// Load the skeleton from file here.
	std::ifstream file(filename);

	if (!file.is_open()) {
		std::cerr << "Error: Could not open the file: " << filename << '\n';
		return;
	}

	std::string str;
	while (true) {

		if (!std::getline(file, str, ' '))
			break;
		float x = std::stof(str);

		if (!std::getline(file, str, ' '))
			break;
		float y = std::stof(str);

		if (!std::getline(file, str, ' '))
			break;
		float z = std::stof(str);

		if (!std::getline(file, str, '\n'))
			break;
		int parent = std::stoi(str);

		Joint* joint = new Joint;
		m_joints.push_back(joint);
		joint->transform = Matrix4f::translation(x, y, z);
		if (parent >= 0)
		{
			m_joints[parent]->children.push_back(joint);
		}
		else
		{
			assert(m_rootJoint == nullptr);
			m_rootJoint = joint;
		}
	}
}

void SkeletalModel::drawJoints()
{
	// Draw a sphere at each joint. You will need to add a recursive helper function to traverse the joint hierarchy.
	//
	// We recommend using glutSolidSphere( 0.025f, 12, 12 )
	// to draw a sphere of reasonable size.
	//
	// You are *not* permitted to use the OpenGL matrix stack commands
	// (glPushMatrix, glPopMatrix, glMultMatrix).
	// You should use your MatrixStack class
	// and use glLoadMatrix() before your drawing call.
	drawJointsRecursive(m_rootJoint);
}

void SkeletalModel::drawJointsRecursive(const Joint* parent)
{
	m_matrixStack.push(parent->transform);

	glLoadMatrixf(m_matrixStack.top());
	glutSolidSphere(0.025f, 12, 12);
	for (auto child : parent->children)
	{
		drawJointsRecursive(child);
	}

	m_matrixStack.pop();
}

void SkeletalModel::drawSkeleton()
{
	// Draw boxes between the joints. You will need to add a recursive helper function to traverse the joint hierarchy.
	drawSkeletonRecursive(m_rootJoint);
}

void SkeletalModel::drawSkeletonRecursive(const Joint* parent)
{
	m_matrixStack.push(parent->transform);

	for (auto child : parent->children)
	{
		auto childOffset = child->transform.getCol(3).xyz();
		auto distToChild = childOffset.abs();

		if (distToChild > 0.f)
		{
			//we are going to draw cube centered around the origin with size equal 1.0
			//in order for it to form a correct bone it must be transformed in a certain way:
			auto translateToNormalizeZ = Matrix4f::translation(0, 0, 0.5);
			auto lengthScale = Matrix4f::scaling(0.01f, 0.01f, distToChild);

			auto basisZ = childOffset.normalized();
			auto rnd = basisZ != Vector3f::UP ? Vector3f::UP : Vector3f::RIGHT;
			auto basisY = Vector3f::cross(basisZ, rnd).normalized();
			auto basisX = Vector3f::cross(basisY, basisZ).normalized();
			auto rotateToChild = Matrix4f::identity();
			rotateToChild.setSubmatrix3x3(0, 0, Matrix3f(basisX, basisY, basisZ, Vector3f::ZERO));

			auto boneTransformation = rotateToChild * lengthScale * translateToNormalizeZ;

			glLoadMatrixf(m_matrixStack.top() * boneTransformation);

			glutSolidCube(1.0);
		}

		drawSkeletonRecursive(child);
	}

	m_matrixStack.pop();
}

void SkeletalModel::setJointTransform(int jointIndex, float rX, float rY, float rZ)
{
	// Set the rotation part of the joint's transformation matrix based on the passed in Euler angles.
	auto rotateX = Matrix3f(
		1, 0, 0,
		0, cosf(rX), -sinf(rX),
		0, sinf(rX), cosf(rX));

	auto rotateY = Matrix3f(
		cosf(rY), 0, sinf(rY),
		0, 1, 0,
		-sinf(rY), 0, cosf(rY));

	auto rotateZ = Matrix3f(
		cosf(rZ), -sinf(rZ), 0,
		sinf(rZ), cosf(rZ), 0,
		0, 0, 1);

	m_joints[jointIndex]->transform.setSubmatrix3x3(0, 0, rotateZ * rotateY * rotateX);
}


void SkeletalModel::computeBindWorldToJointTransforms()
{
	// 2.3.1. Implement this method to compute a per-joint transform from
	// world-space to joint space in the BIND POSE.
	//
	// Note that this needs to be computed only once since there is only
	// a single bind pose.
	//
	// This method should update each joint's bindWorldToJointTransform.
	// You will need to add a recursive helper function to traverse the joint hierarchy.

	computeBindWorldToJointTransformsRecursive(m_rootJoint, Matrix4f::identity());
}

void SkeletalModel::computeBindWorldToJointTransformsRecursive(Joint* joint, const Matrix4f& parentBindWorldToJointTransform)
{
	joint->bindWorldToJointTransform = joint->transform.inverse() * parentBindWorldToJointTransform; //TODO right order???
	for (auto child : joint->children)
	{
		computeBindWorldToJointTransformsRecursive(child, joint->bindWorldToJointTransform);
	}
}

void SkeletalModel::updateCurrentJointToWorldTransforms()
{
	// 2.3.2. Implement this method to compute a per-joint transform from
	// joint space to world space in the CURRENT POSE.
	//
	// The current pose is defined by the rotations you've applied to the
	// joints and hence needs to be *updated* every time the joint angles change.
	//
	// This method should update each joint's currentJointToWorldTransform.
	// You will need to add a recursive helper function to traverse the joint hierarchy.
	updateCurrentJointToWorldTransformsRecursive(m_rootJoint, Matrix4f::identity());
}

void SkeletalModel::updateCurrentJointToWorldTransformsRecursive(Joint* joint, const Matrix4f& parentCurrentJointToWorldTransform)
{
	joint->currentJointToWorldTransform = parentCurrentJointToWorldTransform * joint->transform; //TODO right order???
	for (auto child : joint->children)
	{
		updateCurrentJointToWorldTransformsRecursive(child, joint->currentJointToWorldTransform);
	}
}

void SkeletalModel::updateMesh()
{
	// 2.3.2. This is the core of SSD.
	// Implement this method to update the vertices of the mesh
	// given the current state of the skeleton.
	// You will need both the bind pose world --> joint transforms.
	// and the current joint --> world transforms.

	for (size_t vertNum = 0; vertNum < m_mesh.bindVertices.size(); ++vertNum)
	{
		Vector3f weightedAveragePoint(0, 0, 0);
		assert(m_mesh.attachments[vertNum].size() == m_joints.size());
		bool weightedAveragePointInit = false;
		for (size_t jointNum = 0; jointNum < m_mesh.attachments[vertNum].size(); ++jointNum)
		{
			auto weight = m_mesh.attachments[vertNum][jointNum];
			if (weight == 0.0f)
				continue;

			auto currWeightedPoint = weight
				* (m_joints[jointNum]->currentJointToWorldTransform
				* (m_joints[jointNum]->bindWorldToJointTransform
				* Vector4f(m_mesh.bindVertices[vertNum], 1)));
			weightedAveragePoint += currWeightedPoint.xyz();
			weightedAveragePointInit = true;
		}

		assert(weightedAveragePointInit);
		m_mesh.currentVertices[vertNum] = weightedAveragePoint;
	}

}

