#pragma once

#include <GL/glew.h>
#include <GL/glu.h>
#include <glm/vec2.hpp>

#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <iostream>
#include <vector>

#include "FileHandler.hpp"
#include "Collision.h"
#include "shader/BaseShader.hpp"

struct HeightmapDimensions {
	int width;
	int height;

	float minHeight;
	float maxHeight;
};

class Heightmap {
	public:
		Heightmap() {
			pos = glm::vec3(0.f);
		}
		Heightmap(const std::string path) {
			pos = glm::vec3(0.f);

			HeightmapDimensions dimensions = generateMesh(path);
			std::cout << dimensions.width << "x" << dimensions.height << "px [" << dimensions.minHeight << "," << dimensions.maxHeight << "]\n";
			setupPhysics(dimensions);
		}
		~Heightmap() {}
		HeightmapDimensions generateMesh(const std::string path) {
			int width;
			int height;
			int channels;
			unsigned char* data = FileHandler::getRawImage(path, width, height, channels);
			if(!data)
				throw std::runtime_error("Heightmap::generateMesh(): Unable to load heightmap at \"" + path + "\"");

			glGenTextures(1, &texture);
			glActiveTexture(GL_TEXTURE0);

			glBindTexture(GL_TEXTURE_2D, texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			GLenum format;
			if(channels == 1)
				format = GL_RED;
			else if(channels == 3)
				format = GL_RGB;
			else if(channels == 4)
				format = GL_RGBA;
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			// Get heightdata for physics heightfield
			heightData.clear();
			float minHeight = data[0]/256.f*4;
			float maxHeight = minHeight;
			for(int y = 0; y < height; y++) {
				for(int x = 0; x < width; x++) {
					float val = data[(y * width + x) * channels] / 256.f * 64.f - 16.f;
					heightData.push_back(val);

					if(val < minHeight)
						minHeight = val;
					else if(val > maxHeight)
						maxHeight = val;
				}
				//std::cout << data[y * width]
			}

			FileHandler::freeImage(data);

			// Create quads, which will be subdivided in the tessellation shaders
			res = 16;	// res^2 is the number of quads generated
			std::vector<float> vertices;
			for(int i = 0; i < res; i++) {
				for(int j = 0; j < res; j++) {
					// Vertex 1 of the quad
					vertices.push_back(-width / 2.f + ((width * i) / (float)res));		// X
					vertices.push_back(0.f);											// Y
					vertices.push_back(-height / 2.f + ((height * j) / (float)res));	// Z
					vertices.push_back(i / (float)res);	// U
					vertices.push_back(j / (float)res);	// V
					
					// Vertex 2 of the quad
					vertices.push_back(-width / 2.f + ((width * (i+1)) / (float)res));	// X
					vertices.push_back(0.f);											// Y
					vertices.push_back(-height / 2.f + ((height * j) / (float)res));	// Z
					vertices.push_back((i+1) / (float)res);	// U
					vertices.push_back(j / (float)res);		// V

					// Vertex 3 of the quad
					vertices.push_back(-width / 2.f + ((width * i) / (float)res));			// X
					vertices.push_back(0.f);												// Y
					vertices.push_back(-height / 2.f + ((height * (j+1)) / (float)res));	// Z
					vertices.push_back(i / (float)res);		// U
					vertices.push_back((j+1) / (float)res);	// V

					// Vertex 4 of the quad
					vertices.push_back(-width / 2.f + ((width * (i+1)) / (float)res));		// X
					vertices.push_back(0.f);												// Y
					vertices.push_back(-height / 2.f + ((height * (j+1)) / (float)res));	// Z
					vertices.push_back((i+1) / (float)res);	// U
					vertices.push_back((j+1) / (float)res);	// V
				}
			}
			#ifdef DEBUG
				std::cout 
					<< "Loaded " << (4 * res * res) << " vertices\n" 
					<< (res * res) << " patches will be processed\n";
			#endif

			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float) * 3));
			glEnableVertexAttribArray(1);

			glPatchParameteri(GL_PATCH_VERTICES, 4);
			glBindVertexArray(0);

			return { width, height, minHeight, maxHeight };
		}
		// TODO Fully implement physics
		void setupPhysics(const HeightmapDimensions dimensions) {
			// Create rigidbody
			bool flipQuadEdges = false;

			btHeightfieldTerrainShape* heightfieldShape = new btHeightfieldTerrainShape(
				dimensions.width, 
				dimensions.height, 
				heightData.data(), 
				dimensions.minHeight, 
				dimensions.maxHeight, 
				1, 
				flipQuadEdges
			);

			heightfieldShape->buildAccelerator();

			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(
				btVector3(
					0.f, 
					(std::abs(dimensions.minHeight) + dimensions.maxHeight) / 2 -16, 
					0.f
				)
			);

			rigidBody = createRigidBody(heightfieldShape, transform, 0.f);
			rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
		}
		void draw(BaseShader& shader, const glm::mat4& view, const float& fov, const bool wireframe) {
			shader.bind();
			shader.setPos(pos);
			shader.setRotation(0.f, { 0.f, 1.f, 0.f });
			shader.setScale(1.f, 1.f, 1.f);
			shader.perspective(view, fov);


			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);
			shader.setInt("heightmap", 0);

			glDisable(GL_BLEND);
			if(wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			
			glBindVertexArray(vao);
			glDrawArrays(GL_PATCHES, 0, 4 * res * res);

			if(wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glEnable(GL_BLEND);
		}
		void setPos(const glm::vec3 pos) {
			this->pos = pos;
		}
		btRigidBody* getRigidBody() {
			return rigidBody;
		}
	private:
		unsigned int res;	// Resolution of terrain
		glm::vec3 pos;

		GLuint vao;
		GLuint vbo;
		GLuint texture;

		std::vector<btScalar> heightData;
		btRigidBody* rigidBody;
};