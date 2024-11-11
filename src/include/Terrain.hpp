#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>
#include <vector>
#include "FileHandler.hpp"
#include "shader/BaseShader.hpp"
#include "StaticBody.hpp"

class Terrain {
	public:
		Terrain() {}
		Terrain(const std::string path) {
			getMesh(path);
			pos.pos = { 0.f, 0.f, 0.f };
		}
		~Terrain() {
			delete rigidBody;
			delete motionState;
			delete heightfieldShape;
		}
		void getMesh(const std::string path) {
			int width;
			int height;
			int channels;
			unsigned char* data = FileHandler::getRawImage(path, width, height, channels);
			if(data == nullptr){
				throw std::runtime_error("Terrain::getMesh(): Unable to load heightmap");
			}

			float yScale = 64.0f / 256.0f;	// Scale the range to [0.f, 64.f]
			float yShift = 16.0f;			// Shift the heights to a certain range([-16.f, 48.f])
			std::vector<float> vertices;	// Vector of consecuative vertex data, stride of 3
			vertices.reserve(width*height * sizeof(float));
			for(unsigned int y = 0; y < height; y++) {
				for(unsigned int x = 0; x < width; x++) {
					int index = (y * width + x) * channels;	// Get pixel index
					
					heightData.push_back(((int)(data[index])) * yScale - yShift);
					
					// Vertex
					vertices.push_back(-height / 2.0f + y);	// X
					vertices.push_back(((int)(data[index])) * yScale - yShift);	// Y
					vertices.push_back(-width / 2.0f + x);	// Z
				}
			}
			FileHandler::freeImage(data);

			// Indicies
			// A strip of triangles consists of two rows of height data
			// Between every two columns of data is a quad
			// Each quad has two trianges, each being different "sides"(top/bottom)
			std::vector<unsigned int> indices;
			for(unsigned int strip = 0; strip < height-1; strip++) {
				for(unsigned int column = 0; column < width; column++) {
					for(unsigned int side = 0; side < 2; side++) {
						indices.push_back(column + width * (strip + side));
					}
				}
			}
			
			NUM_STRIPS = height - 1;	// Number of triangle strips
			NUM_VERTS = width * 2;		// Number of verticies per strip
			#ifdef DEBUG
				std::cout << "Generated terrain from heightmap with " << (NUM_STRIPS * (NUM_VERTS - 2)) << " trianges\n";
			#endif

			// Create OpenGL mesh instance
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			glEnableVertexAttribArray(0);

			glGenBuffers(1, &ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

			glBindVertexArray(0);

			// Create rigidbody
			bool flipQuadEdges = false;

			heightfieldShape = new btHeightfieldTerrainShape(width, height, &heightData[0], -yShift, (yScale*256)-yShift, 1, flipQuadEdges);
			heightfieldShape->setLocalScaling(btVector3(1, 1, 1));

			if(true)
				heightfieldShape->buildAccelerator();

			btTransform transform;
			transform.setIdentity();
			transform.setOrigin(btVector3(width / 2, 0, height / 2));

			motionState = new btDefaultMotionState(transform);
			btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, heightfieldShape, btVector3(0, 0, 0));
			rigidBody = new btRigidBody(rbInfo);
		}
		/**
		 * @brief Prepares the body for rendering and draws itself
		 * @param shader The shader used to draw
		 * @param cameraView The view matrix of the camera
		 * @param cameraFOV The FOV of the camera
		*/
		void draw(BaseShader &shader, const glm::mat4 &cameraView, const float &cameraFOV, const bool wireframe) {
			shader.bind();
			shader.setRotation(pos.rotation, pos.rotationAxis);
			shader.setScale(1.f, 1.f, 1.f);
			shader.setPos(pos.pos);
			shader.perspective(cameraView, cameraFOV);

			// draw mesh
			glBindVertexArray(vao);
			glDisable(GL_CULL_FACE);
			if(wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			for(unsigned int strip = 0; strip < NUM_STRIPS; ++strip){
				glDrawElements(GL_TRIANGLE_STRIP, NUM_VERTS, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * NUM_VERTS * strip));
			}

			if(wireframe)
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glEnable(GL_CULL_FACE);

		}
		btRigidBody* getRigidBody() const {
			return rigidBody;
		}
	private:
		unsigned int NUM_STRIPS;
		unsigned int NUM_VERTS;
		
		GLuint vao;
		GLuint vbo;
		GLuint ebo;
		
		StaticPos pos;

		// TODO Find out why the fuck this is needed for OpenGL to actually render the mesh
		// Removing any of it makes the mesh disappear
		// Even moving 'heightData' to the getMesh function does this
		// Even then the mesh is only rendered sometimes, re-running helps
		std::vector<btScalar> heightData;
		btHeightfieldTerrainShape* heightfieldShape;
		btDefaultMotionState* motionState;
		btRigidBody* rigidBody;
};