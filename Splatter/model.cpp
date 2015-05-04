#include "model.h"


Model::Model(string filename)
{
	readObj(filename);

	orientPoints();
	computeBounds();
	normalize();
	findPointSizes();
}


Model::~Model() {}

int Model::readObj(string filename)
{
	ifstream stream(filename);
	
	if (stream.is_open())
	{
		string line, chunk;
		while (stream >> chunk)
		{
			getline(stream, line);
			if (chunk == "v")
				readVertex(line);
			else if (chunk == "vn")
				readNormal(line);
			else if (chunk == "vt")
				readTexture(line);
			else if (chunk == "f")
				readFace(line);
		}
		// If we have normals or texture coordinates, we need to rearrange everything to avoid multiple indices.
		// TODO: Do this without blowing up the size of the v/n/t buffers.
		if (nBuffer.size() > 0 || tBuffer.size() > 0)
		{
			vector<glm::vec4> vBuffer2;
			vector<glm::vec3> nBuffer2;
			vector<glm::vec2> tBuffer2;
			for (unsigned int i = 0; i < viBuffer.size(); i++)
			{
				vBuffer2.push_back(vBuffer[viBuffer[i]]);
				if (niBuffer.size() > 0)
					nBuffer2.push_back(nBuffer[niBuffer[i]]);
				if (tiBuffer.size() > 0)
					tBuffer2.push_back(tBuffer[tiBuffer[i]]);
			}
			vBuffer = vBuffer2;
			nBuffer = nBuffer2;
			tBuffer = tBuffer2;
		}

		stream.close();
		return 0;
	}
	else return -1;
}

int Model::readVertex(string line)
{
	float x, y, z;
	istringstream s(line);

	if (!(s >> x >> y >> z))
		return -1;
	else
		vBuffer.push_back(glm::vec4(x, y, z, 1.0f));
	return 0;
}

int Model::readNormal(string line)
{
	float x, y, z;
	istringstream s(line);

	if (!(s >> x >> y >> z))
		return -1;
	else
		nBuffer.push_back(glm::vec3(x, y, z));
	return 0;
}

int Model::readTexture(string line)
{
	float x, y;
	istringstream s(line);

	if (!(s >> x >> y))
		return -1;
	else
		tBuffer.push_back(glm::vec2(x, y));
	return 0;
}

int Model::readFace(string line)
{
	istringstream s(line);
	char ch;
	unsigned int x;

	// Always three points to a vertex.
	for (int i = 0; i < 3; i++)
	{
		// It always starts with the vertex, so push that.
		s >> x;
		viBuffer.push_back(x-1);

		// OBJs don't require normal or texture indices, but if they're there grab them.
		if (s.peek() == '/')
		{
			s >> ch;
			ch = s.peek();
			if (ch != '/' && ch != ' ')
			{
				s >> x;
				tiBuffer.push_back(x-1);
				ch = s.peek();
			}
			if (ch == '/')
			{
				s >> ch;
				s >> x;
				niBuffer.push_back(x-1);
			}
		}
	}

	return 0;
}

void Model::orientPoints()
{
	// If we don't have normals, calculate normals.
	if (nBuffer.size() == 0)
	{
		nBuffer.resize(vBuffer.size(), glm::vec3(0.0, 0.0, 0.0));

		for (unsigned int i = 0; i < viBuffer.size(); i += 3)
		{
			int a = viBuffer[i];
			int b = viBuffer[i + 1];
			int c = viBuffer[i + 2];
			glm::vec3 normal = glm::cross(glm::vec3(vBuffer[b]) - glm::vec3(vBuffer[a]), glm::vec3(vBuffer[c]) - glm::vec3(vBuffer[a]));
			nBuffer[a] += normal;
			nBuffer[b] += normal;
			nBuffer[c] += normal;
		}
	}
}

/*
Figure out how big each splat should be.
*/
void Model::findPointSizes()
{
	sBuffer.resize(vBuffer.size(), 0.0f);
	float splatScale = 15.0f;
	float nRoot = 1 / 3.0f;
	int a, b, c;
	for (unsigned int i = 0; i < viBuffer.size(); i += 3)
	{
		a = viBuffer[i];
		b = viBuffer[i + 1];
		c = viBuffer[i + 2];
		
		// Splat size is based on the longest distance to a connecting vertex.
		// TODO: Do this better.
		sBuffer[a] = glm::max(sBuffer[a], splatScale * powf(glm::length(vBuffer[a] - vBuffer[b]), nRoot));
		sBuffer[a] = glm::max(sBuffer[a], splatScale * powf(glm::length(vBuffer[a] - vBuffer[c]), nRoot));
		sBuffer[b] = glm::max(sBuffer[a], splatScale * powf(glm::length(vBuffer[b] - vBuffer[a]), nRoot));
		sBuffer[b] = glm::max(sBuffer[a], splatScale * powf(glm::length(vBuffer[b] - vBuffer[c]), nRoot));
		sBuffer[c] = glm::max(sBuffer[a], splatScale * powf(glm::length(vBuffer[c] - vBuffer[a]), nRoot));
		sBuffer[c] = glm::max(sBuffer[a], splatScale * powf(glm::length(vBuffer[c] - vBuffer[b]), nRoot));
	}
}

/*
Calculate the bounds and midpoint of the model.
*/
void Model::computeBounds()
{
	glm::vec4 leftVertex, rightVertex, topVertex, bottomVertex, closeVertex, farVertex;
	leftVertex = rightVertex = topVertex = bottomVertex = closeVertex = farVertex = vBuffer[0];
	largestValue = 0;
	for (unsigned int i = 0; i < vBuffer.size(); i++)
	{
		// Test bounds.
		if (vBuffer[i].x < leftVertex.x) leftVertex = vBuffer[i];
		if (vBuffer[i].x > rightVertex.x) rightVertex = vBuffer[i];
		if (vBuffer[i].y > topVertex.y) topVertex = vBuffer[i];
		if (vBuffer[i].y < bottomVertex.y) bottomVertex = vBuffer[i];
		if (vBuffer[i].z > closeVertex.z) closeVertex = vBuffer[i];
		if (vBuffer[i].z < farVertex.z) farVertex = vBuffer[i];

		// Test to see if it's the biggest value found.
		if (glm::abs(vBuffer[i].x) > largestValue) largestValue = vBuffer[i].x;
		if (glm::abs(vBuffer[i].y) > largestValue) largestValue = vBuffer[i].y;
		if (glm::abs(vBuffer[i].z) > largestValue) largestValue = vBuffer[i].z;
	}
	midVertex = glm::vec3((leftVertex.x + rightVertex.x) / 2.0f, (bottomVertex.y + topVertex.y) / 2.0f, (closeVertex.z + farVertex.z) / 2.0f);
	modelLength = glm::max(farVertex.z - closeVertex.y, glm::max(rightVertex.x - leftVertex.x, topVertex.y - bottomVertex.y));
	modelScale = 1.0f;
}

/*
Bring all values into the range [-1..1].
*/
void Model::normalize()
{
	for (unsigned int i = 0; i < vBuffer.size(); i++)
	{
		vBuffer[i] = vBuffer[i] / largestValue;
	}
	modelLength = 1.0f;
	midVertex = midVertex / largestValue;
}