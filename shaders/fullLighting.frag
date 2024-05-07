#version 330 core

#define NUM_POINT_LIGHTS 4
#define NUM_SPOTLIGHTS 1

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

out vec4 FragColor;

struct Material {
	sampler2D baseTexture;	// Base texture
	sampler2D decal;		// Decal
	sampler2D specMap;		// Specular map
	
	vec3 ambient;			// Percent(0-1) of base color
	
	float shininess;		// "Shininess" of specular shading
	float decalBias;		// "Weight" of the decal
};

struct DirLight {
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct PointLight {
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

struct Spotlight {
	vec3 position;
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;

	float constant;
	float linear;
	float quadratic;

	float cutOff;		// Cos of the angle from camera
	float outerCutOff;	// Larger cutOff value representing the fade cone
};

uniform Material material;

uniform DirLight dirLight;
uniform PointLight pointLights[NUM_POINT_LIGHTS];
uniform Spotlight spotlights[NUM_SPOTLIGHTS];

uniform vec3 cameraPos;

vec3 calcDirLight(DirLight light, vec3 normal, vec3 cameraDir, vec3 textureColor, vec3 baseTextureColor);
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 cameraDir, vec3 textureColor, vec3 baseTextureColor);
vec3 calcSpotlight(Spotlight light, vec3 normal, vec3 fragPos, vec3 cameraDir, vec3 textureColor, vec3 baseTextureColor);

void main() {
	vec3 result = vec3(0.0);  // Output color
	
	vec3 cameraDir = normalize(cameraPos - FragPos);	// Calculate camera direction vector
	vec3 normal = normalize(Normal);						// Calculate normals
	
	vec3 textureColor = vec3(mix(texture(material.baseTexture, TexCoord), texture(material.decal, TexCoord), material.decalBias));
	vec3 baseTextureColor = vec3(texture(material.specMap, TexCoord));

	// Add directional light calculation(only 1)
	result += calcDirLight(dirLight, normal, cameraDir, textureColor, baseTextureColor);

	// Add all point lights
	for(int i = 0; i < NUM_POINT_LIGHTS; i++) {
		result += calcPointLight(pointLights[i], normal, FragPos, cameraDir, textureColor, baseTextureColor);
	}
	// Add all spotlights
	for(int i = 0; i < NUM_SPOTLIGHTS; i++) {
		result += calcSpotlight(spotlights[i], normal, FragPos, cameraDir, textureColor, baseTextureColor);
	}

	vec3 ambient = material.ambient * textureColor;	// Should be seperate(i think)
	FragColor = vec4(result + ambient, 1.0);
}

// Calculate a directional light's effect
vec3 calcDirLight(DirLight light, vec3 normal, vec3 cameraDir, vec3 textureColor, vec3 baseTextureColor) {
	vec3 lightDir = normalize(-light.direction);

	// Diffuse Shading	
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.diffuse * diff * textureColor;

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(cameraDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * baseTextureColor;

	return (diffuse + specular);
}

// Calculate a point light's effect
vec3 calcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 cameraDir, vec3 textureColor, vec3 baseTextureColor) {
	vec3 lightDir = normalize(light.position - fragPos);

	// Diffuse Shading
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse  = light.diffuse * diff * textureColor;

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(cameraDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * baseTextureColor;

	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	diffuse *= attenuation;
	specular *= attenuation;

	return (diffuse + specular);
}

// Calculate a spotlight's effect
vec3 calcSpotlight(Spotlight light, vec3 normal, vec3 fragPos, vec3 cameraDir, vec3 textureColor, vec3 baseTextureColor) {
	vec3 lightDir = normalize(light.position - fragPos);

	float theta = dot(lightDir, normalize(-light.direction));	// Angle between lightDir and the light facing vector
	if(theta < light.outerCutOff) return vec3(0.f, 0.f, 0.f);	// Return as quickly as possible if the pixel is out of the cone
	
	float epsilon   = light.cutOff - light.outerCutOff;							// Cos diff between the inner and outer cones(used for soft edges)
	float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);	// Intensity of the spotlight(used for soft edges)

	// Diffuse Shading
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse  = light.diffuse * diff * textureColor;

	// Specular Shading
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(cameraDir, reflectDir), 0.0), material.shininess);
	vec3 specular = light.specular * spec * baseTextureColor;

	// Attenuation
	float distance = length(light.position - fragPos);
	float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

	diffuse *= attenuation * intensity;
	specular *= attenuation * intensity;

	return (diffuse + specular);
}