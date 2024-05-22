#pragma once

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

#include <GL/glew.h>
#include <GL/glu.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <variant>
#include <functional>

#include "shader/TextShader.hpp"

/**
 * @brief UI Text
*/
struct Text {
	std::string text;	// String
	glm::vec2 pos;		// 2D position on the screen, top-left corner of the text, origin in bottom left-corner
	glm::vec3 color;	// RGB, from 0-1, inclusive
	float scale;		// Scale of 48 pixels
	bool visible = true;

	Text(std::string text, glm::vec2 pos, glm::vec3 color, float scale, bool visible = true) : text(text), pos(pos), color(color), scale(scale), visible(visible) {}
	Text() {}
};

/**
 * @brief Dynamic UI Text
 * @note Holds a weak_ptr to the dynamic value
 * @note Use '<%>' in place where the value would go
*/
template<typename T>
struct DynamicText : public Text {
	std::weak_ptr<T> dynamicVal;	// Weak_ptr to object or value to track

	DynamicText(const std::shared_ptr<T>& val) : dynamicVal(val) {}
	DynamicText(const std::shared_ptr<T>& val, Text base) : Text(base), dynamicVal(val) {}
	virtual std::string getVal() const {
		if(dynamicVal.expired()){ return "NULL"; }
		std::stringstream ss;
		if(std::is_same<bool, T>::value)
			ss << std::boolalpha;	// Actually print true or false
		ss << *dynamicVal.lock();
		return ss.str();
	}
};

/**
 * @brief Dynamic UI Text with a modifier
 * @note Holds a function object to change its state, depending on dynamicVal
 * @note The funciton object is called automatically when using getVal()
*/
template<typename T>
struct DynamicTextFunct : public DynamicText<T> {
	std::function<Text*()> mod;	// Function object which can modify the state of the struct, depending on the dynamicVal

	DynamicTextFunct(const std::function<Text*(DynamicTextFunct<T>&)> modifier, 
	 const std::shared_ptr<T>& val) : DynamicText<T>(val) {
		mod = [this, modifier]() { return modifier(*this); };
	}
	DynamicTextFunct(const std::function<Text*(DynamicTextFunct<T>&)> modifier, 
	 const std::shared_ptr<T>& val, const Text base) : DynamicText<T>(val, base) {
		mod = [this, modifier]() { return modifier(*this); };
	}
	DynamicTextFunct(const std::function<Text*(DynamicTextFunct<T>&)> modifier, 
	 const DynamicText<T> base) : DynamicText<T>(base) {
		mod = [this, modifier]() { return modifier(*this); };
	}
	std::string getVal() const override {
		std::cout << "OVERRIDE " << this->text << "\n";
		Text* a = mod();	// Will always run before drawing because 'getVal()' is always called
		delete a;
		std::cout << "END OVERRIDE " << this->text << "\n";
		return DynamicText<T>::getVal();	// Return the dynamicVal
	}
};

/**
 * @brief Freetype Char
 * @note 
*/
struct FChar {
    GLuint textureID;	// Character texture ID
    glm::ivec2 size;	// Size of character
    glm::ivec2 bearing;	// Offset from baseline to left/top of character
    long int advance;	// Offset to advance to next character
};

// MORE MORE MORE
using DynamicTextTypes = std::variant<DynamicText<bool>, DynamicText<int>, DynamicText<Uint32>, DynamicText<float>, DynamicText<std::string>, 
			DynamicTextFunct<bool>, DynamicTextFunct<int>, DynamicTextFunct<Uint32>, DynamicTextFunct<float>, DynamicTextFunct<std::string>>;

class UI {
	public:
		UI() {
			#ifdef WIN32
				loadFChars(128, "c:\\Windows\\Fonts\\arial.ttf");
			#else
				loadFChars(128, "/home/main/.local/share/fonts/common-web/Arial.TTF");
			#endif
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
			
			glBindTexture(GL_TEXTURE_2D, 0);

			return true;
		}
		/**
		 * @brief Add a Text struct to the Text vector
		*/
		void addTextElement(const Text element) {
			std::string::const_iterator charIter;
			for(charIter = element.text.begin(); charIter != element.text.end(); charIter++){
				if((fChars.find(*charIter)) == fChars.end()){
					std::cerr << "Character \"" << *charIter << "\" not found, skipping" << std::endl;
					/* I have no idea if this would work:
					 * std::cerr << "Character \"" << *charIter << "\" not found. Using \"-\"" << std::endl;
					 * element.text.replace(charIter, 1, '-');*/
				}
			}
			textElements.push_back(std::make_unique<Text>(element));
		}
		/**
		 * @brief Add a DynamicText struct to the DynamicText vector
		*/
		template<typename T>
		void addTextElement(const DynamicText<T> element) {
			std::string::const_iterator charIter;
			for(charIter = element.text.begin(); charIter != element.text.end(); charIter++){
				if((fChars.find(*charIter)) == fChars.end()){
					std::cerr << "Character \"" << *charIter << "\" not found, skipping" << std::endl;
				}
			}
			dynamicTextElements.push_back(std::make_unique<DynamicTextTypes>(element));
		}
		/**
		 * @brief Add a DynamicTextFunct struct to the DynamicText vector
		*/
		template<typename T>
		void addTextElement(const DynamicTextFunct<T> element) {
			std::string::const_iterator charIter;
			for(charIter = element.text.begin(); charIter != element.text.end(); charIter++){
				if((fChars.find(*charIter)) == fChars.end()){
					std::cerr << "Character \"" << *charIter << "\" not found, skipping" << std::endl;
				}
			}
			dynamicTextElements.push_back(std::make_unique<DynamicTextTypes>(element));
		}
		/**
		 * @brief Draw text elements on the screen
		*/
		void drawTextElements(TextShader& shader) {
			// Regular text elements
			for(const auto& element : textElements) {
				if(element.get()->visible)
					renderText(shader, *element);
			}

			// Dynamic text elements
			for(const auto& element : dynamicTextElements) {
				Text out;
				auto temp = element.get();
				
				// Struct vals
				std::string val;

				std::visit([&val](const auto& value) { val = value.getVal(); }, *temp);	// Get the dynamic value as a string
				std::visit([&out](const auto& arg) { out = static_cast<Text>(arg); }, *temp);	// Get the base
				if(!out.visible) std::cout<<"A\n";	// Skip if not visible

				// Replace text with dynamic value
				size_t start_pos = out.text.find("<%>");
				if(start_pos != std::string::npos)
					out.text.replace(start_pos, size_t(3), val);
				renderText(shader, out);
			}
		}

		/**
		 * @brief Draws arbitrary text on the screen
		*/
		void renderText(TextShader &shader, Text &text) {
			shader.bind();
			shader.setColor(text.color);
			shader.setPos(glm::vec3(640.f, 480.f, 0.f));

			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(shader.getVAO());

			// Draw each character
			float x = text.pos.x;	// X position of the next character
			std::string::const_iterator charIter;
			for(charIter = text.text.begin(); charIter != text.text.end(); charIter++) {
				if((fChars.find(*charIter)) == fChars.end()) continue;	// Character not in map
				FChar ch = fChars.find(*charIter)->second;

				float xpos = x + ch.bearing.x * text.scale;
				float ypos = text.pos.y - (ch.size.y - ch.bearing.y) * text.scale;	// Account for the distance below the baseline(size.y - bearing.y) for 'g', 'p', etc.

				float width = ch.size.x * text.scale;
				float height = ch.size.y * text.scale;

				// Update VBO
				float vertices[6][4] = {
					{ xpos, ypos + height, 0.f, 0.f },            
					{ xpos, ypos, 0.f, 1.f },
					{ xpos + width, ypos, 1.f, 1.f },

					{ xpos, ypos + height, 0.f, 0.f },
					{ xpos + width, ypos, 1.f, 1.f },
					{ xpos + width, ypos + height, 1.f, 0.f }           
				};
				
				glBindTexture(GL_TEXTURE_2D, ch.textureID);
				glBindBuffer(GL_ARRAY_BUFFER, shader.getVBO());
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
				glBindBuffer(GL_ARRAY_BUFFER, 0);

				glDrawArrays(GL_TRIANGLES, 0, 6);

				// Adjust next X position for the next character
				x += (ch.advance >> 6) * text.scale; // Bitshift by 6 to get value in pixels
			}
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	private:
		std::map<char, FChar> fChars;
		std::vector<std::unique_ptr<Text>> textElements;
		std::vector<std::unique_ptr<DynamicTextTypes>> dynamicTextElements;
};