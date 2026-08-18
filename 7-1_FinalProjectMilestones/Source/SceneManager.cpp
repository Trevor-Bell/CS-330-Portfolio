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
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
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
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
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

/***********************************************************
  *  DefineObjectMaterials()
  *
  *  This method is used for configuring the various material
  *  settings for all of the objects within the 3D scene.
  ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	// material simulates a dark wood surface
	OBJECT_MATERIAL blackWood;
	// diffuse lighting
	blackWood.diffuseColor = glm::vec3(0.15f, 0.15f, 0.17f);
	// specular lighting
	blackWood.specularColor = glm::vec3(0.05f, 0.05f, 0.06f);
	blackWood.shininess = 8.0f;

	// material tag
	blackWood.tag = "table";

	m_objectMaterials.push_back(blackWood);

	// material simulates a ceramic surface
	OBJECT_MATERIAL ceramicBase;
	// diffuse lighting
	ceramicBase.diffuseColor = glm::vec3(0.55f, 0.65f, 0.75f);
	// specular lighting
	ceramicBase.specularColor = glm::vec3(0.45f, 0.45f, 0.45f);
	ceramicBase.shininess = 45.0f;

	// material tag
	ceramicBase.tag = "lampBase";

	m_objectMaterials.push_back(ceramicBase);

	// material simulates a ceramic surface
	OBJECT_MATERIAL ceramicNeck;
	// diffuse lighting
	ceramicNeck.diffuseColor = glm::vec3(0.55f, 0.28f, 0.20f);
	// specular lighting
	ceramicNeck.specularColor = glm::vec3(0.55f, 0.45f, 0.40f);
	ceramicNeck.shininess = 60.0f;

	// material tag
	ceramicNeck.tag = "lampNeck";

	m_objectMaterials.push_back(ceramicNeck);

	// material simulates a plastic surface
	OBJECT_MATERIAL shade;
	// diffuse lighting
	shade.diffuseColor = glm::vec3(0.90f, 0.88f, 0.85f);
	// specular lighting
	shade.specularColor = glm::vec3(0.70f, 0.65f, 0.60f);
	shade.shininess = 55.0f;

	// material tag
	shade.tag = "lampShade";

	m_objectMaterials.push_back(shade);

	// material simulates a metal surface
	OBJECT_MATERIAL button;
	// diffuse lighting
	button.diffuseColor = glm::vec3(0.30f, 0.32f, 0.35f);
	// specular lighting
	button.specularColor = glm::vec3(0.80f, 0.82f, 0.85f);
	button.shininess = 85.0f;   
	
	// material tag
	button.tag = "button";

	m_objectMaterials.push_back(button);

	// material simulates a wooden surface
	OBJECT_MATERIAL lockbox;
	// diffuse lighting
	lockbox.diffuseColor = glm::vec3(0.45f, 0.48f, 0.50f);
	// specular lighting
	lockbox.specularColor = glm::vec3(0.70f, 0.72f, 0.75f);
	lockbox.shininess = 40.0f;

	// material tag
	lockbox.tag = "lockbox";

	m_objectMaterials.push_back(lockbox);

	// material simulates a glass surface
	OBJECT_MATERIAL glass;
	// diffuse lighting
	glass.diffuseColor = glm::vec3(0.1f, 0.1f, 0.15f);
	// specular lighting
	glass.specularColor = glm::vec3(0.9f, 0.9f, 0.95f);
	glass.shininess = 96.0f;

	// material tag
	glass.tag = "glass";

	m_objectMaterials.push_back(glass);

	// material simulates a ceramic surface
	OBJECT_MATERIAL cup;
	// diffuse lighting
	cup.diffuseColor = glm::vec3(0.92f, 0.92f, 0.90f);
	// specular lighting
	cup.specularColor = glm::vec3(0.65f, 0.65f, 0.65f);
	cup.shininess = 28.0f;

	// material tag
	cup.tag = "cup";

	m_objectMaterials.push_back(cup);

}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/

	// primary neutral light placed above and to the right of scene
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.6f, -1.0f, -0.3f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.25f, 0.22f, 0.20f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.9f, 0.85f, 0.8f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.4f, 0.38f, 0.35f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// secondary warm light placed inside lamp
	m_pShaderManager->setVec3Value("pointLights[1].position", 0.0f, 2.1f, 0.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.35f, 0.22f, 0.10f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 1.0f, 0.65f, 0.30f);
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.9f, 0.55f, 0.25f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	/*** STUDENTS - add the code BELOW for loading the textures that ***/
	/*** will be used for mapping to objects in the 3D scene. Up to  ***/
	/*** 16 textures can be loaded per scene. Refer to the code in   ***/
	/*** the OpenGL Sample for help.
	***/
	CreateGLTexture("../../Utilities/textures/blackWood.jpg", "table");
	CreateGLTexture("../../Utilities/textures/lampBase.jpg", "lamp_base");
	CreateGLTexture("../../Utilities/textures/lampNeck.jpg", "lamp_neck");
	CreateGLTexture("../../Utilities/textures/lampShade.jpg", "lamp_shade");
	CreateGLTexture("../../Utilities/textures/stainless.jpg", "metal");
	CreateGLTexture("../../Utilities/textures/stainless_end.jpg", "metal_end");
	CreateGLTexture("../../Utilities/textures/wood.jpg", "wood");
	CreateGLTexture("../../Utilities/textures/glass.jpg", "glass");
	CreateGLTexture("../../Utilities/textures/cup.jpg", "cup");

	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

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
	// load the textures for the 3D scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("table");
	// set the material for the next draw command
	SetShaderMaterial("table");
	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// lamp
	// base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.0f, 0.25f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("lamp_base");
	// set the material for the next draw command
	SetShaderMaterial("lampBase");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// neck
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.5f, 1.25f, 0.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.25f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("lamp_neck");
	// set the material for the next draw command
	SetShaderMaterial("lampNeck");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// shade tapered cylinder
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.25f, 1.0f, 1.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 1.5f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("lamp_shade");
	// set the material for the next draw command
	SetShaderMaterial("lampShade");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// shade sphere
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.75f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 2.10f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("lamp_shade");
	// set the material for the next draw command
	SetShaderMaterial("lampShade");
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	// button torus
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.05f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.75f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("metal");
	// set the material for the next draw command
	SetShaderMaterial("button");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	// button sphere
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.05f, 0.05f, 0.05f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.75f, 0.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("metal_end");
	// set the material for the next draw command
	SetShaderMaterial("button");
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	// lockbox
	// bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 0.6f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.3f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("wood");
	// set the material for the next draw command
	SetShaderMaterial("lockbox");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.5f, 0.2f, 1.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-4.0f, 0.7f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("wood");
	// set the material for the next draw command
	SetShaderMaterial("lockbox");
	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();

	// latches
	// left latch
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.02f, 0.4f, 0.02f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.8f, 0.6f, 2.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("metal");
	// set the material for the next draw command
	SetShaderMaterial("button");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// right latch
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.02f, 0.4f, 0.02f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = -45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-3.25f, 0.6f, 1.95f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("metal");
	// set the material for the next draw command
	SetShaderMaterial("button");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// teapot
	// base bottom
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.4f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("glass");
	// set the material for the next draw command
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// base top
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.75f, 0.4f, 0.75f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.4f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("glass");
	// set the material for the next draw command
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// lid
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.6f, 0.1f, 0.6f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.9f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("glass");
	// set the material for the next draw command
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// lid handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.0f, 0.95f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("glass");
	// set the material for the next draw command
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawSphereMesh();

	// handle
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.75f, 0.5f, 1.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("glass");
	// set the material for the next draw command
	SetShaderMaterial("glass");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTorusMesh();

	// cup
	// base
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.25f, 0.05f, 0.25f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.0f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("cup");
	// set the material for the next draw command
	SetShaderMaterial("cup");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();

	// body
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.4f, 0.4f, 0.4f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.45f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("cup");
	// set the material for the next draw command
	SetShaderMaterial("cup");
	// draw the mesh with transformation values
	m_basicMeshes->DrawTaperedCylinderMesh();

	// rim
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(0.405f, 0.025f, 0.405f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.45f, 2.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// SetShaderColor(1, 1, 1, 1);
	// set the texture for the next draw command
	SetShaderTexture("cup");
	// set the material for the next draw command
	SetShaderMaterial("cup");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh();
}