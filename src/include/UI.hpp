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

/**
 * @brief Freetype Char
 * @note 
*/
struct FChar {
	unsigned int textureID;	// Character texture ID
	glm::ivec2 size;		// Size of character
	glm::ivec2 bearing;		// Offset from baseline to left/top of character
	long int advance;		// Offset to advance to next character
};

/**
 * @brief Holds a character map
*/
struct Font {
	std::map<char, FChar> charMap;

	Font() {}
	Font(std::map<char, FChar> fChars) : charMap(fChars) {}
};

/**
 * @brief UI Text
*/
class Text {
	public:
		Text(const std::string& text, const glm::vec2& pos, const glm::vec3& color, const float& scale, const bool& visible = true)
			: text(text), pos(pos), color(color), scale(scale), visible(visible) {}
		Text() : text(""), pos(glm::vec2(0.f, 0.f)), color(glm::vec3(1.f, 1.f, 1.f)), scale(1.f), visible(true) {}
		virtual void draw(TextShader &shader) {
			if(!visible || font.expired()) return;

			auto fChars = font.lock().get()->charMap;

			shader.bind();
			shader.setColor(color);
			shader.setPos(glm::vec3(640.f, 480.f, 0.f));

			glActiveTexture(GL_TEXTURE0);
			glBindVertexArray(shader.getVAO());

			// Draw each character
			float x = pos.x;	// X position of the each character
			std::string::const_iterator charIter;
			for(charIter = text.begin(); charIter != text.end(); charIter++) {
				if((fChars.find(*charIter)) == fChars.end()) continue;	// Character not in map
				FChar ch = fChars.find(*charIter)->second;

				float xpos = x + ch.bearing.x * scale;
				float ypos = pos.y - (ch.size.y - ch.bearing.y) * scale;	// Account for the distance below the baseline(size.y - bearing.y) for 'g', 'p', etc.

				float width = ch.size.x * scale;
				float height = ch.size.y * scale;

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
				x += (ch.advance >> 6) * scale; // Bitshift by 6 to get value in pixels
			}
			glBindVertexArray(0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		virtual ~Text() = default;
		
		std::string text;
		glm::vec2 pos;
		glm::vec3 color;
		float scale;
		bool visible;

		std::weak_ptr<Font> font;
};

/**
 * @brief Dynamic UI Text
 * @note Holds a weak_ptr to a dynamic value
 * @note If desired, use "<%>" in place where the value sould go in the text
*/
class DynamicText : public Text {
	using ValueVariant = std::variant<std::shared_ptr<int>, std::shared_ptr<bool>, std::shared_ptr<float>, std::shared_ptr<std::string>, std::shared_ptr<void>>;

	public:
		template <typename T>
		DynamicText(const std::string& text, const glm::vec2& pos, const glm::vec3& color, const float& scale, const bool& visible,
			std::shared_ptr<T> dynamicVal, std::function<void(DynamicText&)> mutator)
			: Text(text, pos, color, scale, visible), dynamicVal(dynamicVal), mutator(mutator) {}
		template <typename T>
		DynamicText(Text text, std::shared_ptr<T> dynamicVal, std::function<void(DynamicText&)> mutator)
			: Text(text), dynamicVal(dynamicVal), mutator(mutator) {}
		void draw(TextShader &shader) override {
			if(mutator)
				mutator(*this);
			
			if(!this->visible) return;

			// TODO find a better way
			std::ostringstream oss;
			if(auto a = std::static_pointer_cast<int>(dynamicVal.lock()))
				oss << *a;
			else if(auto a = std::static_pointer_cast<float>(dynamicVal.lock()))
				oss << *a;
			else if(auto a = std::static_pointer_cast<bool>(dynamicVal.lock()))
				oss << *a;
			else if(auto a = std::static_pointer_cast<std::string>(dynamicVal.lock()))
				oss << *a;
			else
				Text::draw(shader);
			
			// Replace text with dynamic value
			std::string backup = text;
			size_t start_pos = text.find("<%>");
			if(start_pos != std::string::npos)
				text.replace(start_pos, size_t(3), oss.str());

			Text::draw(shader);
			text = backup;	// Restore the special character
		}

		uint32_t index;
		std::weak_ptr<void> dynamicVal;
		std::function<void(DynamicText&)> mutator;
};

class UI {
	public:
		UI() {
			#ifdef WIN32
				loadFChars(128, "c:\\Windows\\Fonts\\Arial.ttf");
			#else
				loadFChars(128, "/home/main/.local/share/fonts/common-web/Arial.TTF");
			#endif
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

			std::map<char, FChar> fChars;
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

			std::string fontName;
			size_t index = font.find_last_of('.');
			if(index != std::string::npos)
				fontName = font.substr(0, index);
			
			#ifdef WIN32
				index = fontName.find_last_of('\\');
			#else
				index = fontName.find_last_of('/');
			#endif

			if(index != std::string::npos)
				fontName = fontName.substr(index+1);
			// Add font to list
			fonts.insert(
				std::pair<std::string, std::shared_ptr<Font>>(
					fontName, 
					std::make_unique<Font>(Font(fChars))
			));

			// Free
			FT_Done_Face(face);
			FT_Done_FreeType(ft);
			
			glBindTexture(GL_TEXTURE_2D, 0);

			return true;
		}
		/**
		 * @brief Add a Text element struct to the UI
		*/
		bool addTextElement(std::unique_ptr<Text> element, std::string font = "Arial") {
			// Get font
			auto temp = UI::fonts.find(font);
			if(temp != UI::fonts.end()){
				element.get()->font = temp->second;
			} else {
				std::cerr << "UI::addTextElement(): Unable to load font, \"" << font << "\", loading arial" << std::endl;
				temp = UI::fonts.find("Arial");
				if(temp != UI::fonts.end()){
					element.get()->font = temp->second;
				} else {
					std::cerr << "UI::addTextElement(): Unable to load arial font" << std::endl;
					return false;
				}
			}

			elements.push_back(std::move(element));
			return true;
		}
		/**
		 * @brief Draw text elements on the screen
		*/
		void drawTextElements(TextShader& shader) {
			// Regular text elements
			for(const auto& element : elements) {
				element->draw(shader);
			}
		}
		
	private:
		std::vector<std::unique_ptr<Text>> elements;
		static std::map<std::string, std::shared_ptr<Font>> fonts;
};

std::map<std::string, std::shared_ptr<Font>> UI::fonts;