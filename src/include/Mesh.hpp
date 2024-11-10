#pragma once

#include <GL/glew.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>

#include "shader/BaseShader.hpp"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

struct Texture {
	GLuint id;
	
	std::string type;
	std::string path;
};

class Mesh {
	public:
		Mesh(std::vector<Vertex> vertices, std::vector<GLuint> indices, std::vector<Texture> textures, const std::string name) {
			this->name     = name;
			this->vertices = vertices;
			this->indices  = indices;
			this->textures = textures;

			// Setup
			glGenVertexArrays(1, &vao);
			glGenBuffers(1, &vbo);
			glGenBuffers(1, &ebo);
		
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);

			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);  

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

			// Set vertex positions
			glEnableVertexAttribArray(0);	
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			// Set normals
			glEnableVertexAttribArray(1);	
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
			// Set texture coordinates
			glEnableVertexAttribArray(2);	
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));

			glBindVertexArray(0);
		}
		void draw(BaseShader &shader) {
			GLuint diffuseNr = 1;
			GLuint specularNr = 1;
			
			for(GLuint i = 0; i < textures.size(); i++) {
				glActiveTexture(GL_TEXTURE0 + i);

				std::string name = textures[i].type;
				std::string number;	// The texture number(N in diffuseTextureN)
				
				// Compare type
				if(name == "diffuseTexture"){
					number = std::to_string(diffuseNr);
					diffuseNr++;
				} else if(name == "specMap"){
					number = std::to_string(specularNr);
					specularNr++;
				}
				
				shader.setInt(("material." + name).c_str(), i);
				glBindTexture(GL_TEXTURE_2D, textures[i].id);
			}

			// Draw
			glBindVertexArray(vao);
			glDrawElements(GL_TRIANGLES, static_cast<GLuint>(indices.size()), GL_UNSIGNED_INT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindVertexArray(0);
		}
		const std::vector<GLuint>& getIndices() const {
			return indices;
		}
		const std::vector<Texture>& getTextures() const {
			return textures;
		}
		GLuint getVAO() const {
			return vao;
		}
		GLuint getVBO() const {
			return vbo;
		}
		GLuint getEBO() const {
			return ebo;
		}
		std::string getName() const {
			return name;
		}
	private:
		GLuint vao;
		GLuint vbo;
		GLuint ebo;

		std::string name;
		std::vector<Vertex> vertices;
		std::vector<GLuint> indices;
		std::vector<Texture> textures;
};
