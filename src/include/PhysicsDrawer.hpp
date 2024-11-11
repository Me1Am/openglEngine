#pragma once

#include <GL/glew.h>
#include <GL/glu.h>

#include <iostream>
#include <memory>

#include "UI.hpp"
#include "shader/ColliderShader.hpp"


class PhysicsDrawer : public btIDebugDraw {
	public:
		PhysicsDrawer() : ui(nullptr), debugMode(DBG_NoDebug) {
			initOpenGL();
		}
		PhysicsDrawer(UI* ui) : ui(ui), debugMode(DBG_NoDebug) {
			initOpenGL();
		}
		~PhysicsDrawer() {
			glDeleteBuffers(1, &vboLine);
			glDeleteBuffers(1, &vboLineColor);
			glDeleteBuffers(1, &vboTriangle);
			glDeleteBuffers(1, &vboTriangleColor);
			glDeleteVertexArrays(1, &vao);

			delete ui;
		}
		/**
		 * @brief Pushes a line's vertices to the internal buffer
		 * @note Same as drawLine(from, to, color, color);
		 */
		void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override {
			drawLine(from, to, color, color);
		}
		/**
		 * @brief Pushes a line's vertices to the internal buffer with a color gradient
		 */
		void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) override {
			vertices.push_back(from.getX());
			vertices.push_back(from.getY());
			vertices.push_back(from.getZ());
			colors.push_back(fromColor.getX());
			colors.push_back(fromColor.getY());
			colors.push_back(fromColor.getZ());

			vertices.push_back(to.getX());
			vertices.push_back(to.getY());
			vertices.push_back(to.getZ());
			colors.push_back(toColor.getX());
			colors.push_back(toColor.getY());
			colors.push_back(toColor.getZ());
		}
		/**
		 * @brief Pushes a triange's vertices to the internal buffer
		 * @note `alpha` is discarded
		 */
		void drawTriangle(const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& color, btScalar alpha) override {
			triangles.push_back(a.getX());
			triangles.push_back(a.getY());
			triangles.push_back(a.getZ());
			triColors.push_back(color.getX());
			triColors.push_back(color.getY());
			triColors.push_back(color.getZ());

			triangles.push_back(b.getX());
			triangles.push_back(b.getY());
			triangles.push_back(b.getZ());
			triColors.push_back(color.getX());
			triColors.push_back(color.getY());
			triColors.push_back(color.getZ());

			triangles.push_back(c.getX());
			triangles.push_back(c.getY());
			triangles.push_back(c.getZ());
			triColors.push_back(color.getX());
			triColors.push_back(color.getY());
			triColors.push_back(color.getZ());
		}
		/**
		 * @brief Pushes a line between collision contact points to the buffer 
		 */
		void drawContactPoint(const btVector3& pointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {
			btVector3 to = pointOnB + normalOnB * distance;
			drawLine(pointOnB, to, color);
		}
		/**
		 * @brief Draws text onto the screen
		 * @param location The position of the text
		 * @param textString The text to draw
		 * @note Only "draws" 2D text, ignores z component of `location`
		 * @note Does not actually draw the text, just pushes it to the UI class
		 * @note Outputs to terminal when `ui` is null and when DEBUG is defined
		 */
		void draw3dText(const btVector3& location, const char* textString) override {
			if(ui == nullptr){
				#ifdef DEBUG
					std::cerr << "PhysicsDrawer()::draw3dText(): Attempted to draw text but \'ui\' is null\n";
					std::cerr << "    Text: \"" << textString << "\"\n";
				#endif
				return;
			}

			Text text;
			text.text = textString;
			ui->addTextElement(std::make_unique<Text>(text));
		}
		/**
		 * @brief Renders all lines and triangles added by `drawLine()` and `drawTriangle()`
		 */
		void flushLines() override {
			if(vertices.empty() && triangles.empty()) return;

			// Bind the shader and set up uniforms
			shader.bind();

			glBindVertexArray(vao);

			// Upload line vertices
			if(!vertices.empty()) {
				glBindBuffer(GL_ARRAY_BUFFER, vboLine);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glEnableVertexAttribArray(0);

				// Upload line colors
				glBindBuffer(GL_ARRAY_BUFFER, vboLineColor);
				glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GLfloat), colors.data(), GL_DYNAMIC_DRAW);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glEnableVertexAttribArray(1);

				glDrawArrays(GL_LINES, 0, vertices.size() / 3);

				// Clear the buffers
				vertices.clear();
				colors.clear();
			}

			// Upload triangle vertices
			if(!triangles.empty()) {
				glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);
				glBufferData(GL_ARRAY_BUFFER, triangles.size() * sizeof(GLfloat), triangles.data(), GL_DYNAMIC_DRAW);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glEnableVertexAttribArray(0);

				// Upload triangle colors
				glBindBuffer(GL_ARRAY_BUFFER, vboTriangleColor);
				glBufferData(GL_ARRAY_BUFFER, triColors.size() * sizeof(GLfloat), triColors.data(), GL_DYNAMIC_DRAW);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
				glEnableVertexAttribArray(1);

				glDrawArrays(GL_TRIANGLES, 0, triangles.size() / 3);

				// Clear the buffers
				triangles.clear();
				triColors.clear();
			}

			glBindVertexArray(0);
		}
		/** 
		 * @brief Sets the view and projection matrixes for the shader
		 */
		void setCamera(const glm::mat4& cameraView, const float& fov) {
			shader.bind();
			shader.perspective(cameraView, fov);
		}
		/**
		 * @brief Prints to cerr
		 */
		void reportErrorWarning(const char* warningString) override {
			std::cerr << warningString << '\n';
		}
		/**
		 * @brief Sets the current debug mode
		 */
		void setDebugMode(int debugMode) override {
			this->debugMode = debugMode;
		}
		/**
		 * @brief Gets the current debug mode
		 * @return The current debug mode, see `btIDebugDraw::DebugDrawModes`
		 */
		int getDebugMode() const override {
			return debugMode;
		}
	private:
		/**
		 * @brief Initialize OpenGL components and the shader
		 */
		void initOpenGL() {
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vboLine);
			glGenBuffers(1, &vboLineColor);
			glGenBuffers(1, &vboTriangle);
			glGenBuffers(1, &vboTriangleColor);

			glBindVertexArray(0);

			shader.loadProgram();
		}

		std::vector<GLfloat> vertices;	// Vector for line segments
		std::vector<GLfloat> colors;	// Vector for line segment colors
		std::vector<GLfloat> triangles;	// Vector for triangles
		std::vector<GLfloat> triColors;	// Vector for triangle colors

		int debugMode;
		GLuint vboLine, vboLineColor, vboTriangle, vboTriangleColor, vao;

		UI* ui;	// Pointer to an existing UI class
		ColliderShader shader;
};
