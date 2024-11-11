
#pragma once

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <variant>
#include <functional>

#include "shader/TextShader.hpp"

class TextSystem {
	public:
		TextSystem() {
			shader.loadProgram("../shaders/text2.vert", "../shaders/text2.frag");
			glGenBuffers(1, &vbo);
		}
		~TextSystem() {}
		bool loadFChars(int numChars, std::string font) {
			FT_Library ft;
			if(FT_Init_FreeType(&ft)){
				std::cerr << "UI::loadFChars(): Unable to initialize FreeType library" << std::endl;
				return false;
			}
			FT_Face face;
			if(FT_New_Face(ft, font.c_str(), 0, &face)){
				std::cerr << "UI::loadFChars(): Unable to load font \"" << font << "\"" << std::endl;
				return false;
			}

			FT_Set_Pixel_Sizes(face, 0, 48);		// Set height and width of characters

			unsigned int w;
			unsigned int h;
			for(unsigned char c = 0; c < numChars; c++) {
				if(FT_Load_Char(face, c, FT_LOAD_RENDER)){
					std::cerr << "UI::loadFChars(): Unable to load character #" << c << std::endl;
					continue;
				}

				w += face->glyph->bitmap.width;
				h = std::max(h, face->glyph->bitmap.rows);
			}

			atlas.width = w;
			atlas.height = h;

			GLuint tex;
			glActiveTexture(GL_TEXTURE0);
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int x = 0;
			for(unsigned char c = 0; c < numChars; c++) {
				if(FT_Load_Char(face, c, FT_LOAD_RENDER))
					continue;

				glTexSubImage2D(
					GL_TEXTURE_2D,
					0,
					x,
					0,
					face->glyph->bitmap.width,
					face->glyph->bitmap.rows,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->glyph->bitmap.buffer
				);

				// Set character information
				chars[c].ax = face->glyph->advance.x >> 6;
				chars[c].ay = face->glyph->advance.y >> 6;

				chars[c].bw = face->glyph->bitmap.width;
				chars[c].bh = face->glyph->bitmap.rows;

				chars[c].bl = face->glyph->bitmap_left;
				chars[c].bt = face->glyph->bitmap_top;

				chars[c].tx = (float)x / w;

				x += face->glyph->bitmap.width;
			}

			glBindTexture(GL_TEXTURE_2D, 0);

			// Free
			FT_Done_Face(face);
			FT_Done_FreeType(ft);

			return true;
		}
		void renderText(const std::string& text, const glm::vec2& pos, const glm::vec2& scale, const glm::vec3& color) {
			shader.bind();
			shader.setColor(color);
			shader.setPos(1920, 1080, 0);

			glBindBuffer(GL_ARRAY_BUFFER, vbo);

			int n = 0;
			float x = pos.x;
			float y = pos.y;
			std::vector<glm::vec4> points;
			for(int i = 0; i < (6 * text.length()); i++) { points.push_back(glm::vec4(0.f)); }
			for(const char* p = text.c_str(); *p; p++) {
				float x2 =  x + chars[*p].bl * scale.x;
				float y2 = -y - chars[*p].bt * scale.y;
				float w = chars[*p].bw * scale.x;
				float h = chars[*p].bh * scale.y;

				x += chars[*p].ax * scale.x;
				y += chars[*p].ay * scale.y;

				if(!w || !h)
					continue;

				points[n++] = glm::vec4(x2,     -y2, 	 chars[*p].tx, 							0);
				points[n++] = glm::vec4(x2 + w, -y2, 	 chars[*p].tx + chars[*p].bw / atlas.width, 0);
				points[n++] = glm::vec4(x2,     -y2 - h, chars[*p].tx, 							chars[*p].bh / atlas.height);
				points[n++] = glm::vec4(x2 + w, -y2, 	 chars[*p].tx + chars[*p].bw / atlas.width, 0);
				points[n++] = glm::vec4(x2,     -y2 - h, chars[*p].tx, 							chars[*p].bh / atlas.height);
				points[n++] = glm::vec4(x2 + w, -y2 - h, chars[*p].tx + chars[*p].bw / atlas.width, chars[*p].bh / atlas.height);
			}

			glBufferData(GL_ARRAY_BUFFER, sizeof(points), points.data(), GL_DYNAMIC_DRAW);
			glDrawArrays(GL_TRIANGLES, 0, n);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		};
	private:
		struct Atlas {
			unsigned int width;
			unsigned int height;
		} atlas;

		struct FreeTypeChar {
			float ax; // advance.x
			float ay; // advance.y

			float bw; // bitmap.width;
			float bh; // bitmap.rows;

			float bl; // bitmap_left;
			float bt; // bitmap_top;

			float tx; // x offset of glyph in texture coordinates
		};

		TextShader shader;
		GLuint vbo;
		std::array<FreeTypeChar, 128> chars;
};








/*
struct FreeTypeChar {
	int letterIndex;		// Character texture ID
	glm::ivec2 size;		// Size of character
	glm::ivec2 bearing;		// Offset from baseline to left/top of character
	long int advance;		// Offset to advance to next character
};

class TextSystem {
	public:
		TextSystem() {
			for(int i = 0; i < ARRAY_LIMIT; i++) {
				transforms.push_back(glm::mat4x4(1.f));
				letterMap.push_back(0);
			}
		}
		~TextSystem() {}
		bool loadFChars(int numChars, std::string font) {
			FT_Library ft;
			if(FT_Init_FreeType(&ft)){
				std::cerr << "UI::loadFChars(): Unable to initialize FreeType library" << std::endl;
				return false;
			}
			FT_Face face;
			if(FT_New_Face(ft, font.c_str(), 0, &face)){
				std::cerr << "UI::loadFChars(): Unable to load font \"" << font << "\"" << std::endl;
				return false;
			}

			FT_Set_Pixel_Sizes(face, 256, 256);		// Set height and width of characters
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disable byte-alignment

			glGenTextures(1, &textureArray);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8, 256, 256, 128, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

			for(unsigned char c = 0; c < numChars; c++) {
				if(FT_Load_Char(face, c, FT_LOAD_RENDER)){
					std::cerr << "UI::loadFChars(): Unable to load character #" << c << std::endl;
					continue;
				}

				glTexSubImage3D(
					GL_TEXTURE_2D_ARRAY,
					0,
					0,
					0,
					int(c),
					face->glyph->bitmap.width,
					face->glyph->bitmap.rows,
					1,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->glyph->bitmap.buffer
				);

				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				FreeTypeChar character = {
					int(c),
					glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
					glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
					face->glyph->advance.x
				};
				fChars.insert(std::pair<char, FreeTypeChar>(c, character));
			}

			// Free
			FT_Done_Face(face);
			FT_Done_FreeType(ft);

			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

			return true;
		}
		void renderText(TextShader& shader, const std::string text, float x, float y, float scale, const glm::vec3 color) {
			scale = scale * 48.f / 256.f;

			shader.bind();
			shader.setColor(color);
			shader.setPos(glm::vec3(1920.f, 1080.f, 0.f));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
			glBindVertexArray(shader.getVAO());
			glBindBuffer(GL_ARRAY_BUFFER, shader.getVBO());

			// Draw each character
			int copyX = x;
			int workingIndex = 0;
			std::string::const_iterator charIter;
			for(charIter = text.begin(); charIter != text.end(); charIter++) {
				if((fChars.find(*charIter)) == fChars.end()) continue;	// Character not in map
				if(workingIndex == ARRAY_LIMIT-1) break;

				FreeTypeChar ch = fChars.find(*charIter)->second;

				if(*charIter == '\n'){
					y -= ((ch.size.y)) * 1.3 * scale;
					x = copyX;
					continue;
				} else if(*charIter == '\t'){
					x += (ch.advance >> 6) * scale * 4;
					continue;
				} else if(*charIter == ' '){
					x += (ch.advance >> 6) * scale;
					continue;
				}

				float xpos = x + ch.bearing.x * scale;
				float ypos = y - (ch.size.y - ch.bearing.y) * scale;	// Account for the distance below the baseline(size.y - bearing.y) for 'g', 'p', etc.
				float width = 256 * scale;	// ch.size.x
				float height = 256 * scale;	// ch.size.y

				transforms[workingIndex] = glm::translate(glm::mat4x4(1.f), glm::vec3(xpos, ypos, 0.f)) * glm::scale(glm::mat4x4(1.f), glm::vec3(width, height, 0));
				letterMap[workingIndex] = ch.letterIndex;

				draw(shader, workingIndex);

				// Adjust next X position for the next character
				x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels
				workingIndex++;
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
		}
	private:
		void draw(TextShader& shader, const int& workingIndex) {
			glUniformMatrix4fv(glGetUniformLocation(shader.getProgramID(), "transform"), workingIndex, GL_FALSE, &transforms[0][0][0]);
			glUniform1iv(glGetUniformLocation(shader.getProgramID(), "letterMap"), workingIndex, &letterMap[0]);
			glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, workingIndex);
		}

		std::map<char, FreeTypeChar> fChars;
		std::vector<glm::mat4x4> transforms;
		std::vector<int> letterMap;
		GLuint textureArray;

		const int ARRAY_LIMIT = 200;
};
*/
/*
struct FreeTypeChar {
	unsigned int textureID;	// Character texture ID
	glm::ivec2 size;		// Size of character
	glm::ivec2 bearing;		// Offset from baseline to left/top of character
	long int advance;		// Offset to advance to next character
};

class TextSystem {
	public:
		TextSystem() {}
		~TextSystem() {}
		bool loadFChars(int numChars, std::string font) {
			FT_Library ft;
			if(FT_Init_FreeType(&ft)){
				std::cerr << "UI::loadFChars(): Unable to initialize FreeType library" << std::endl;
				return false;
			}
			FT_Face face;
			if(FT_New_Face(ft, font.c_str(), 0, &face)){
				std::cerr << "UI::loadFChars(): Unable to load font \"" << font << "\"" << std::endl;
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

				FreeTypeChar character = {
					texture, 
					glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows), 
					glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), 
					face->glyph->advance.x
				};
				fChars.insert(std::pair<char, FreeTypeChar>(c, character));
			}

			// Free
			FT_Done_Face(face);
			FT_Done_FreeType(ft);

			glBindTexture(GL_TEXTURE_2D, 0);

			return true;
		}
		void renderText(TextShader& shader, const std::string text, float x, float y, const float scale, const glm::vec3 color) {
			shader.bind();
			shader.setColor(color);
			shader.setPos(glm::vec3(1920.f, 1080.f, 0.f));

			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(shader.getVAO());

			// Draw each character
			int copyX = x;
			std::string::const_iterator charIter;
			for(charIter = text.begin(); charIter != text.end(); charIter++) {
				if((fChars.find(*charIter)) == fChars.end()) continue;	// Character not in map

				FreeTypeChar ch = fChars.find(*charIter)->second;

				if(*charIter == '\n'){
					y -= ((ch.size.y)) * 1.3 * scale;
					x = copyX;
					continue;
				} else if(*charIter == '\t'){
					x += (ch.advance >> 6) * scale * 4;
					continue;
				} else if(*charIter == ' '){
					x += (ch.advance >> 6) * scale;
					continue;
				}

				float xpos = x + ch.bearing.x * scale;
				float ypos = y - (ch.size.y - ch.bearing.y) * scale;	// Account for the distance below the baseline(size.y - bearing.y) for 'g', 'p', etc.
				float width = ch.size.x * scale;
				float height = ch.size.y * scale;

				transform = glm::translate(glm::mat4x4(1.f), glm::vec3(xpos, ypos, 0.f)) * glm::scale(glm::mat4x4(1.f), glm::vec3(width, height, 0));
				glUniformMatrix4fv(glGetUniformLocation(shader.getProgramID(), "transform"), 1, GL_FALSE, glm::value_ptr(transform));

				glBindTexture(GL_TEXTURE_2D, ch.textureID);
				glBindBuffer(GL_ARRAY_BUFFER, shader.getVBO());

				glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 1);

				// Adjust next X position for the next character
				x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	private:
		std::map<char, FreeTypeChar> fChars;
		glm::mat4x4 transform = glm::mat4x4(1.f);
};
*/
