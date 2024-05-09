#pragma once

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <GL/glew.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <map>

#include "shader/TextShader.hpp"

struct Text {
	std::string text;
	glm::vec2 pos;

};

/**
 * @brief Freetype Char
*/
struct FChar {
    uint textureID;		// Character texture ID
    glm::ivec2 size;	// Size of character
    glm::ivec2 bearing;	// Offset from baseline to left/top of character
    long int advance;	// Offset to advance to next character
};

class UI {
	public:
		UI() {
			loadFChars(128, "../assets/fonts/arial.ttf");
		}
		UI(std::string font) {
			loadFChars(128, font);
		}
		~UI() {}
		/**
		 * @brief Loads Freetype characters to into FChars and the fChars map
		 * @param numChars The number of ASCII characters to load
		 * @return Whether the operation was successful
		*/
		bool loadFChars(int numChars, std::string font) {
			FT_Library ft;
			if(FT_Init_FreeType(&ft)){
				std::cerr << "UI::loadFChars(): Unable to initialize FreeType library" << std::endl;
				return false;
			}
			FT_Face face;
			if(FT_New_Face(ft, ("fonts/" + font).c_str(), 0, &face)){
				std::cerr << "UI::loadFChars(): Unable to load font '" << font << "'" << std::endl;
				return false;
			}

			FT_Set_Pixel_Sizes(face, 0, 48);	// Set height and width of characters
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment

			for(unsigned char c = 0; c < numChars; c++) {
				if(FT_Load_Char(face, c, FT_LOAD_RENDER)){
					std::cerr << "UI::loadFChars(): Unable to load character #" << c << std::endl;
					continue;
				}

				GLuint texture;
				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_2D, texture);
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_RED,
					face->glyph->bitmap.width,
					face->glyph->bitmap.rows,
					0,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->glyph->bitmap.buffer
				);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				
				// Save character
				FChar character = {
					texture, 
					glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
					glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
					face->glyph->advance.x
				};
				fChars.insert(std::pair<char, FChar>(c, character));
			}

			// Free
			FT_Done_Face(face);
			FT_Done_FreeType(ft);

			return true;
		}
		/**
		 * @brief Draws arbitrary text on the screen as UI elements
		*/
		void drawText(BaseShader &shader, std::string text, glm::vec2 pos, glm::vec3 color) {}

		void RenderText(TextShader &shader, std::string text, float x, float y, float scale, glm::vec3 color) {
			shader.bind();
			shader.setColor(color);

			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(shader.getVAO());

			// Draw each character
			std::string::const_iterator charIter;
			for(charIter = text.begin(); charIter != text.end(); charIter++) {
				FChar ch = fChars[*charIter];

				float xpos = x + ch.bearing.x * scale;
				float ypos = y - (ch.size.y - ch.bearing.y) * scale;	// Account for the distance below the baseline(size.y - bearing.y) for 'g', 'p', etc.

				float width = ch.size.x * scale;
				float height = ch.size.y * scale;

				// Update VBO
				float vertices[6][4] = {
					{ xpos, ypos + height, 0.0f, 0.0f },            
					{ xpos, ypos, 0.0f, 1.0f },
					{ xpos + width, ypos, 1.0f, 1.0f },

					{ xpos, ypos + height, 0.0f, 0.0f },
					{ xpos + width, ypos, 1.0f, 1.0f },
					{ xpos + width, ypos + height, 1.0f, 0.0f }           
				};

				glBindTexture(GL_TEXTURE_2D, ch.textureID);
				glBindBuffer(GL_ARRAY_BUFFER, shader.getVBO());
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				glDrawArrays(GL_TRIANGLES, 0, 6);

				// Adjust next X position for the next character
				x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels
			}
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	private:
		std::map<char, FChar> fChars;
};