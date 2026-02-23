///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ,
	glm::vec3 offset)
{
	// variables for this method
	glm::mat4 model;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ + offset);

	model = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, model);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{

	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// All textures
	CreateGLTexture("textures/saltshaker.png", "shaker");
	CreateGLTexture("textures/cap_sides.png", "cap_sides");
	CreateGLTexture("textures/cap_top.png", "cap_top");
	CreateGLTexture("textures/cap_torus.png", "cap_torus");
	CreateGLTexture("textures/butter_face.png", "butter_front");
	CreateGLTexture("textures/butter_side1.png", "butter_left");
	CreateGLTexture("textures/butter_side2.png", "butter_right");
	CreateGLTexture("textures/butter_side3.png", "butter_back");
	CreateGLTexture("textures/butter_top.png", "butter_top");
	CreateGLTexture("textures/butter_bottom.png", "butter_bottom");
	CreateGLTexture("textures/Tile.png", "tile");
	CreateGLTexture("textures/wood_top.png", "wood_tex");

	BindGLTextures();

	// All shapes used
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// =========================================================
	// PLATFORM (plane)
	// =========================================================
	DrawTexturedMesh(MeshType::BOX,
		glm::vec3(60.0f, 1.5f, 15.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f), 
		"wood_tex", 1.0f, 1.0f, "wood", false, true, false);


	// =========================================================
	// SALT SHAKERS (left and right)
	// 1) tapered cylinder = glass body
	// 2) cylinder         = cap
	// 3) torus            = base ring
	// =========================================================

	// --- LEFT SALTSHAKER ---
	// --- Glass body ---
	DrawTexturedMesh(MeshType::TAPERED_CYLINDER,
		glm::vec3(2.0f, 4.5f, 2.0f),
		glm::vec3(-10.0f, 0.75f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		"shaker", 1.0f, 1.0f, "glass", false, false, true);
	// --- Cap ---
	DrawTexturedMesh(MeshType::CYLINDER,
		glm::vec3(1.1f, 0.6f, 1.1f),
		glm::vec3(-10.0f, 4.75f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		"cap_sides",1.0f,1.0f, "metal", false, false, true);
	DrawTexturedMesh(MeshType::CYLINDER,
		glm::vec3(1.1f, 0.6f, 1.1f),
		glm::vec3(-10.0f, 4.75f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		"cap_top", 1.0f, 1.0f, "metal", true, false, false);
	// --- Base ring ---
	DrawMesh(MeshType::TORUS,
		glm::vec3(1.05f, 1.05f, 1.05f),
		glm::vec3(-10.0f, 4.7f, 0.0f),
		glm::vec3(90.0f, 0.0f, 0.0f),
		glm::vec4(0.447f, 0.447f, 0.447f, 1.0f), "metal");

	// --- RIGHT SALTSHAKER ---
	// --- Glass body ---
	DrawTexturedMesh(MeshType::TAPERED_CYLINDER,
		glm::vec3(2.0f, 4.5f, 2.0f),
		glm::vec3(10.0f, 0.75f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		"shaker", 1.0f, 1.0f, "glass", false, false, true);
	// --- Cap ---
	DrawTexturedMesh(MeshType::CYLINDER,
		glm::vec3(1.1f, 0.6f, 1.1f),
		glm::vec3(10.0f, 4.75f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		"cap_sides", 1.0f, 1.0f, "metal", false, false, true);
	DrawTexturedMesh(MeshType::CYLINDER,
		glm::vec3(1.1f, 0.6f, 1.1f),
		glm::vec3(10.0f, 4.75f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		"cap_top", 1.0f, 1.0f, "metal", true, false, false);
	// --- Base ring ---
	DrawMesh(MeshType::TORUS,
		glm::vec3(1.05f, 1.05f, 1.05f),
		glm::vec3(10.0f, 4.7f, 0.0f),
		glm::vec3(90.0f, 0.0f, 0.0f),
		glm::vec4(0.447f, 0.447f, 0.447f, 1.0f), "metal");

	// =============================
	// HANGING LAMP 
	// 1) cylinder = Cord
	// 2) cone     = Shade
	// 3) sphere   = Bulb
	// =============================
	glm::vec3 lampBase = glm::vec3(0.0f, 18.5f, 0.0f);
	float drop = 6.0f;    
	glm::vec3 coneBase = lampBase + glm::vec3(0.0f, -(drop + 0.9f), 0.0f);
	glm::vec4 cordCol = glm::vec4(0.55f, 0.55f, 0.55f, 1.0f);
	glm::vec4 shadeCol = glm::vec4(0.65f, 0.65f, 0.65f, 1.0f);
	glm::vec4 bulbCol = glm::vec4(1.00f, 0.95f, 0.75f, 1.0f);

	// --- CORD ---
	DrawMesh(
		MeshType::CYLINDER,
		glm::vec3(0.1f, drop, 0.1f),
		lampBase,
		glm::vec3(180.0f, 0.0f, 0.0f),
		cordCol,
		"metal"
	);
	// --- SHADE ---
	DrawMesh(
		MeshType::CONE,
		glm::vec3(1.0f, 1.0f, 1.0f),
		coneBase,
		glm::vec3(0.0f, 0.0f, 0.0f),
		shadeCol,
		"plastic",
		false, true, false
	);
	// --- BULB ---
	DrawMesh(
		MeshType::SPHERE,
		glm::vec3(0.5f, 0.5f, 0.5f),
		coneBase,
		glm::vec3(0.0f, 0.0f, 0.0f),
		bulbCol,
		"plastic"
	);

	// =============================
	// WALL (plane)
	// =============================
	DrawTexturedMesh(
		MeshType::PLANE,
		glm::vec3(30.0f, 1.0f, 18.0f),
		glm::vec3(0.0f, 17.0f, -7.5f),
		glm::vec3(90.0f, 0.0f, 0.0f),
		"tile",
		12.0f, 6.0f,
		"tile",
		true, true, true
	);


	// =========================================================
	// BUTTER CHARACTER
	// 1) box              = main body
	// 2) cylinder         = legs and arms
	// =========================================================

	glm::vec3 butterBase = glm::vec3(0.0f, 1.5f + 1.125f, 0.0f);
	glm::vec3 butterBodSize = glm::vec3(2.0f, 2.25f, 1.5f);
	glm::vec3 butterBodRot = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec4 butterBaseColor = glm::vec4(0.95f, 0.85f, 0.25f, 1.0f);

	// --- BODY --- defines all sides of the box to add each texture
	DrawTexturedMesh(
		MeshType::BOX_FRONT,
		butterBodSize,
		butterBase ,
		butterBodRot,
		"butter_front",
		1.0f, 1.0f,
		"plastic"
	);
	DrawTexturedMesh(
		MeshType::BOX_LEFT,
		butterBodSize,
		butterBase,
		butterBodRot,
		"butter_left",
		1.0f, 1.0f,
		"plastic"
	);
	DrawTexturedMesh(
		MeshType::BOX_RIGHT,
		butterBodSize,
		butterBase,
		butterBodRot,
		"butter_right",
		1.0f, 1.0f,
		"plastic"
	);
	DrawTexturedMesh(
		MeshType::BOX_BACK,
		butterBodSize,
		butterBase,
		butterBodRot,
		"butter_back",
		1.0f, 1.0f,
		"plastic"
	);
	DrawTexturedMesh(
		MeshType::BOX_BOTTOM,
		butterBodSize,
		butterBase,
		butterBodRot,
		"butter_bottom",
		1.0f, 1.0f,
		"plastic"
	);
	DrawTexturedMesh(
		MeshType::BOX_TOP,
		butterBodSize,
		butterBase,
		butterBodRot,
		"butter_top",
		1.0f, 1.0f,
		"plastic"
	);

	// --- LEGS ---
	float bodyBottomY = butterBase.y - (butterBodSize.y * 0.5f);
	glm::vec3 legSize = glm::vec3(0.25f, bodyBottomY, 0.25f);
	// Leg offsets - spacing under the body
	float legOffsetX = butterBodSize.x * 0.22f;
	float legOffsetZ = butterBodSize.z * 0.18f;

	// Left leg
	DrawMesh(
		MeshType::CYLINDER,
		legSize,
		glm::vec3(butterBase.x - legOffsetX, (legSize.y * 0.5f), butterBase.z + legOffsetZ),
		glm::vec3(0.0f, 0.0f, 0.0f),
		butterBaseColor,
		"plastic"
	);
	// Right leg
	DrawMesh(
		MeshType::CYLINDER,
		legSize,
		glm::vec3(butterBase.x + legOffsetX, (legSize.y * 0.5f), butterBase.z + legOffsetZ),
		glm::vec3(0.0f, 0.0f, 0.0f),
		butterBaseColor,
		"plastic"
	);

	// --- ARMS ---
	glm::vec3 armSize = glm::vec3(0.20f, 1.0f, 0.20f);
	glm::vec3 armRot = glm::vec3(180.0f, 0.0f, 0.0f);
	// Arm offsets: arms just outside body width(armOffsetX), forward offset so they are visible from front(armOffsetZ)
	float armOffsetX = (butterBodSize.x * 0.5f) + (armSize.x * 0.5f) - 0.05f;
	float armOffsetZ = butterBodSize.z * 0.15f;

	// Left arm
	DrawMesh(
		MeshType::CYLINDER,
		armSize,
		glm::vec3(butterBase.x - armOffsetX, butterBase.y, butterBase.z + armOffsetZ),
		armRot,
		butterBaseColor,
		"plastic"
	);
	// Right arm
	DrawMesh(
		MeshType::CYLINDER,
		armSize,
		glm::vec3(butterBase.x + armOffsetX, butterBase.y, butterBase.z + armOffsetZ),
		armRot,
		butterBaseColor,
		"plastic"
	);

}

// lighting and material setup
void SceneManager::DefineObjectMaterials()
{

	// Below we define a few materials which in use in the scene for certain objects
	OBJECT_MATERIAL plastic;
	plastic.diffuseColor = glm::vec3(0.80f, 0.80f, 0.80f);
	plastic.specularColor = glm::vec3(0.15f, 0.15f, 0.15f);
	plastic.shininess = 8.0f;
	plastic.tag = "plastic";
	m_objectMaterials.push_back(plastic);

	OBJECT_MATERIAL tileMaterial;
	tileMaterial.diffuseColor = glm::vec3(0.85f, 0.85f, 0.85f);
	tileMaterial.specularColor = glm::vec3(0.75f, 0.75f, 0.75f);
	tileMaterial.shininess = 32.0f;
	tileMaterial.tag = "tile";
	m_objectMaterials.push_back(tileMaterial);

	OBJECT_MATERIAL metal;
	metal.diffuseColor = glm::vec3(0.55f, 0.55f, 0.55f);
	metal.specularColor = glm::vec3(0.90f, 0.90f, 0.90f);
	metal.shininess = 64.0f;
	metal.tag = "metal";
	m_objectMaterials.push_back(metal);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.6f, 0.5f, 0.2f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.2f, 0.2f);
	woodMaterial.shininess = 1.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.2f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.8f);
	glassMaterial.shininess = 10.0;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

}

void SceneManager::SetupSceneLights()
{

	if (NULL == m_pShaderManager) return; // safety check

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Light 0: hanging lamp
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 11.05f, 2.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.04f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.00f, 0.85f, 0.55f); 
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.25f, 0.22f, 0.18f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// Light 1: soft fill
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.0f, 2.5f, 4.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.03f, 0.03f, 0.03f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.45f, 0.45f, 0.45f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.10f, 0.10f, 0.10f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

}

//* Helper functions.
void SceneManager::DrawTexturedMesh(MeshType type, glm::vec3 scale, glm::vec3 pos, glm::vec3 rot, const std::string& textureTag, float uTile, float vTile, const std::string& materialTag, bool top, bool bottom, bool sides)
{
	SetShaderTexture(textureTag);
	SetTextureUVScale(uTile, vTile);
	SetTransformations(scale, rot.x, rot.y, rot.z, pos);
	draw(type, top, bottom, sides);	
}

void SceneManager::DrawMesh(MeshType type, glm::vec3 scale, glm::vec3 pos, glm::vec3 rot, glm::vec4 col, const std::string& materialTag, bool top, bool bottom, bool sides)
{
	SetTransformations(scale, rot.x, rot.y, rot.z, pos);
	
		SetShaderColor(col.r, col.g, col.b, col.a);
	
		SetShaderMaterial(materialTag);
	draw(type);
}

void SceneManager::draw(MeshType type, bool top, bool bottom, bool sides)
{
	switch (type)
	{
	case MeshType::PLANE:            m_basicMeshes->DrawPlaneMesh(); break;
	case MeshType::BOX:              m_basicMeshes->DrawBoxMesh(); break;
	case MeshType::BOX_FRONT:        m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front); break;
	case MeshType::BOX_BACK:        m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::back); break;
	case MeshType::BOX_BOTTOM:        m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::bottom); break;
	case MeshType::BOX_TOP:        m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::top); break;
	case MeshType::BOX_RIGHT:        m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::right); break;
	case MeshType::BOX_LEFT:        m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left); break;
	case MeshType::CONE:             m_basicMeshes->DrawConeMesh(bottom); break;
	case MeshType::CYLINDER:         m_basicMeshes->DrawCylinderMesh(top, bottom, sides); break;
	case MeshType::PRISM:            m_basicMeshes->DrawPrismMesh(); break;
	case MeshType::PYRAMID3:          m_basicMeshes->DrawPyramid3Mesh(); break;
	case MeshType::PYRAMID4:          m_basicMeshes->DrawPyramid4Mesh(); break;
	case MeshType::SPHERE:           m_basicMeshes->DrawSphereMesh(); break;
	case MeshType::TAPERED_CYLINDER: m_basicMeshes->DrawTaperedCylinderMesh(top, bottom, sides); break;
	case MeshType::TORUS:            m_basicMeshes->DrawTorusMesh(); break;
	}
}

