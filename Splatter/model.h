#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <glm\glm.hpp>

using namespace std;

class Model
{
private:
	// Buffers for storing vertices, normals, faces.
	vector<glm::vec4> vBuffer;
	vector<glm::vec2> tBuffer;
	vector<glm::vec3> nBuffer;
	vector<float> sBuffer;
	vector<unsigned int> viBuffer;
	vector<unsigned int> tiBuffer;
	vector<unsigned int> niBuffer;

	// Extra info for the model.
	glm::vec3 midVertex;
	float modelLength;
	float modelScale;
	float largestValue;

public:
	Model(string filename);
	~Model();

	int readObj(string filename);
	int readVertex(string line);
	int readNormal(string line);
	int readTexture(string line);
	int readFace(string line);

	void orientPoints();
	void computeBounds();
	void findPointSizes();
	void normalize();
	
	vector<glm::vec4>* getVertices() { return &vBuffer; }
	vector<glm::vec2>* getUVs() { return &tBuffer; }
	vector<glm::vec3>* getNormals() { return &nBuffer; }
	vector<float>* getSizes() { return &sBuffer; }
	vector<unsigned int>* getVertexIndices() { return &viBuffer; }
	vector<unsigned int>* getTextureIndices() { return &tiBuffer; }
	vector<unsigned int>* getNormalIndices() { return &niBuffer; }

	void setScale(float s) { modelScale = s; }

	glm::vec3 getMidpoint() { return midVertex; }
	float getLength() { return modelLength; }
	float getScale() { return modelScale; }
};

