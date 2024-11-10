#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <string_view>
#include <vector>

#include "Mesh.hpp"
#include "shader/BaseShader.hpp"
#include "FileHandler.hpp"

class Model {
	public:
		/** 
		 * Default Constructor
		 */
		Model() {}
		/**
		 * @brief Loads a model from a path
		 * @param path A c_str pointing to the model location
		*/
		Model(const char* path) {
			if(!initialize(path)){
				std::cerr << "Model::Model(): Unable to initialize" << std::endl;
				return;
			}
			delete[] path;
		}
		/**
		 * @brief Initializes the model, loads it and its textures
		 * @param path The path to the model
		 * @note If path is set to an empty string, it just sets 'directory' to "" and exits
		*/
		bool initialize(const char* path) {
			// Don't load if using empty path
			if(*path == '\0'){
				directory = "";
				return true;
			}

			// Load Model
			Assimp::Importer importer;	// Assimp model importer(abstracts model formats into its own)

			/* Loads the given model
			 *  'aiProcess_Triangulate' converts all faces into triagnes
			 *  'aiProcess_GenNormals' creates normals if there are none
			 *  'aiProcess_FlipUVs' causes issues, dont use
			 */
			const aiScene *scene = importer.ReadFile(	
				path, 
				aiProcess_Triangulate | 
				aiProcess_GenSmoothNormals
			);

			// Error check
			if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
				std::cerr << "Model::Initialize(): " << importer.GetErrorString() << std::endl;
				return false;
			}
			
			directory = std::string(path).substr(0, std::string(path).find_last_of('/'));	// Set model's working directory(assuming textures are there)
			
			processNode(scene->mRootNode, scene);

			return true;
		}
		/**
		 * @brief Loops through the vector of meshes and calls their draw function
		 * @param shader The shader to use when rendering
		*/
		void draw(BaseShader &shader) {
			for(GLuint i = 0; i < meshes.size(); i++) {
				meshes[i].draw(shader);
			}
		}
		/**
		 * @brief Gets a pointer to the mesh
		 * @param name The name of the desired mesh
		 * @return A pointer to the desired mesh
		*/
		Mesh* getMesh(const std::string name) {
			auto iter = std::find_if(meshes.begin(), meshes.end(), 
				[name](Mesh& mesh) { return mesh.getName() == name; });
			return &meshes[std::distance(meshes.begin(), iter)];
		}
		const std::vector<Mesh>& getMeshes() const {
			return meshes;
		}
	private:
		void processNode(aiNode* node, const aiScene* scene) {
			// Process the node's meshes
			for(GLuint i = 0; i < node->mNumMeshes; i++) {
				aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
				meshes.push_back(processMesh(mesh, scene));
			}

			// Process the node's children
			for(GLuint i = 0; i < node->mNumChildren; i++) {
				processNode(node->mChildren[i], scene);
			}
		}
		Mesh processMesh(aiMesh* mesh, const aiScene* scene) {
			std::vector<Vertex> vertices;
			std::vector<GLuint> indices;
			std::vector<Texture> textures;

			for(GLuint i = 0; i < mesh->mNumVertices; i++){
				Vertex vertex;
				
				// Vertex position
				glm::vec3 vector; 
				vector.x = mesh->mVertices[i].x;
				vector.y = mesh->mVertices[i].y;
				vector.z = mesh->mVertices[i].z;
				vertex.pos = vector;

				// Vertex normal 
				if(mesh->HasNormals()){
					vector.x = mesh->mNormals[i].x;
					vector.y = mesh->mNormals[i].y;
					vector.z = mesh->mNormals[i].z;
					vertex.normal = vector;
				}

				// Vertex texture
				if(mesh->mTextureCoords[0]){
					glm::vec2 vec;
					vec.x = mesh->mTextureCoords[0][i].x;
					vec.y = mesh->mTextureCoords[0][i].y;
					vertex.texCoord = vec;
				} else {
					vertex.texCoord = glm::vec2(0.f, 0.f);
				}

				vertices.push_back(vertex);
			}

			// Indicies
			for(GLuint i = 0; i < mesh->mNumFaces; i++) {
				aiFace face = mesh->mFaces[i];
				
				for(GLuint j = 0; j < face.mNumIndices; j++){
					indices.push_back(face.mIndices[j]);
				}
			}
			
			// Material
			if(mesh->mMaterialIndex >= 0){
				aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
				
				std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "diffuseTexture");
				std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "specMap");
				
				textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
				textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
			}

			return Mesh(vertices, indices, textures, (std::string)mesh->mName.C_Str());
		}
		std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName) {
			std::vector<Texture> textures;
			
			for(GLuint i = 0; i < mat->GetTextureCount(type); i++) {
				aiString str;
				mat->GetTexture(type, i, &str);	// Gets the texture path('i' being an index to list of them)
	
				bool preloaded = false;
				for(GLuint j = 0; j < loadedTextures.size(); j++) {
					// Compare a loaded texture path to this one's
					if(!std::strcmp(loadedTextures[j].path.data(), str.C_Str())){
						textures.push_back(loadedTextures[j]);	// Add the preloaded one
						preloaded = true;	// Skip the regular load process
						break;
					}
				}
				// Only go through the load process if it hasn't been loaded before
				if(!preloaded){
					Texture texture;

					// Texture load preperation
					GLuint textureID;
					glGenTextures(1, &textureID);
					glBindTexture(GL_TEXTURE_2D, textureID);

					// Set struct properties
					bool status = FileHandler::loadImage(directory + '/' + str.C_Str());	// Load texture with local path
					if(!status){ return {}; }	// Failed, return null

					texture.id = textureID;
					texture.type = typeName;
					texture.path = str.C_Str();
					textures.push_back(texture);
					loadedTextures.push_back(texture);	// Add newly loaded texture
				}
			}

			return textures;
		}

		std::vector<Mesh> 		meshes;			// Meshes of the model
		std::vector<Texture> 	loadedTextures;	// Loaded textures, allows reuse between meshes
		std::string 			directory;		// The model base directory
};
