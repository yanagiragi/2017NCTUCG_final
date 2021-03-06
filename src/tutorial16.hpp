#ifndef T16
#define T16

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <iostream>

#include "../include/GL/glew.h"
#include "../include/GL/freeglut.h"
#include "../include/glm/glm.hpp"
#include "../include/glm/gtc/matrix_transform.hpp"

class tutorial16
{
public:

	struct PackedVertex {
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		bool operator<(const PackedVertex that) const {
			return memcmp((void*)this, (void*)&that, sizeof(PackedVertex))>0;
		};
	};

	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;

	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint normalbuffer;
	GLuint elementbuffer;

	// Constructor
	tutorial16() {}

	bool loadOBJ(const char * path, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals)
	{
		printf("Loading OBJ file %s...\n", path);

		std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
		std::vector<glm::vec3> temp_vertices;
		std::vector<glm::vec2> temp_uvs;
		std::vector<glm::vec3> temp_normals;


		FILE * file = fopen(path, "r");
		if (file == NULL) {
			printf("Impossible to open the file ! Are you in the right path ? See Tutorial 1 for details\n");
			getchar();
			return false;
		}

		while (1) {

			char lineHeader[128];
			// read the first word of the line
			int res = fscanf(file, "%s", lineHeader);
			if (res == EOF)
				break; // EOF = End Of File. Quit the loop.

					   // else : parse lineHeader

			if (strcmp(lineHeader, "v") == 0) {
				glm::vec3 vertex;
				fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
				temp_vertices.push_back(vertex);
			}
			else if (strcmp(lineHeader, "vt") == 0) {
				glm::vec2 uv;
				fscanf(file, "%f %f\n", &uv.x, &uv.y);
				uv.y = -uv.y; // Invert V coordinate since we will only use DDS texture, which are inverted. Remove if you want to use TGA or BMP loaders.
				temp_uvs.push_back(uv);
			}
			else if (strcmp(lineHeader, "vn") == 0) {
				glm::vec3 normal;
				fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
				temp_normals.push_back(normal);
			}
			else if (strcmp(lineHeader, "f") == 0) {
				std::string vertex1, vertex2, vertex3;
				unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
				int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
				if (matches != 9) {
					printf("File can't be read by our simple parser :-( Try exporting with other options\n");
					fclose(file);
					return false;

				}
				vertexIndices.push_back(vertexIndex[0]);
				vertexIndices.push_back(vertexIndex[1]);
				vertexIndices.push_back(vertexIndex[2]);
				uvIndices.push_back(uvIndex[0]);
				uvIndices.push_back(uvIndex[1]);
				uvIndices.push_back(uvIndex[2]);
				normalIndices.push_back(normalIndex[0]);
				normalIndices.push_back(normalIndex[1]);
				normalIndices.push_back(normalIndex[2]);
			}
			else {
				// Probably a comment, eat up the rest of the line
				char stupidBuffer[1000];
				fgets(stupidBuffer, 1000, file);
			}

		}

		// For each vertex of each triangle
		for (unsigned int i = 0; i<vertexIndices.size(); i++) {

			// Get the indices of its attributes
			unsigned int vertexIndex = vertexIndices[i];
			unsigned int uvIndex = uvIndices[i];
			unsigned int normalIndex = normalIndices[i];

			// Get the attributes thanks to the index
			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			glm::vec2 uv = temp_uvs[uvIndex - 1];
			glm::vec3 normal = temp_normals[normalIndex - 1];

			// Put the attributes in buffers
			out_vertices.push_back(vertex);
			out_uvs.push_back(uv);
			out_normals.push_back(normal);

		}
		fclose(file);
		return true;
	}

	bool getSimilarVertexIndex_fast(PackedVertex & packed, std::map<PackedVertex, unsigned short> & VertexToOutIndex, unsigned short & result)
	{
		std::map<PackedVertex, unsigned short>::iterator it = VertexToOutIndex.find(packed);
		if (it == VertexToOutIndex.end()) {
			return false;
		}
		else {
			result = it->second;
			return true;
		}
	}

	void indexVBO(std::vector<glm::vec3> & in_vertices, std::vector<glm::vec2> & in_uvs, std::vector<glm::vec3> & in_normals, std::vector<unsigned short> & out_indices, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals)
	{
		std::map<PackedVertex, unsigned short> VertexToOutIndex;

		// For each input vertex
		for (unsigned int i = 0; i<in_vertices.size(); i++) {

			PackedVertex packed = { in_vertices[i], in_uvs[i], in_normals[i] };


			// Try to find a similar vertex in out_XXXX
			unsigned short index;
			bool found = getSimilarVertexIndex_fast(packed, VertexToOutIndex, index);

			if (found) { // A similar vertex is already in the VBO, use it instead !
				out_indices.push_back(index);
			}
			else { // If not, it needs to be added in the output data.
				out_vertices.push_back(in_vertices[i]);
				out_uvs.push_back(in_uvs[i]);
				out_normals.push_back(in_normals[i]);
				unsigned short newindex = (unsigned short)out_vertices.size() - 1;
				out_indices.push_back(newindex);
				VertexToOutIndex[packed] = newindex;
			}
		}
	}

	void Init()
	{
		indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

		glGenBuffers(1, &uvbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

		glGenBuffers(1, &normalbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

		glGenBuffers(1, &elementbuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);
	}

	void RenderShadowMap16()
	{
		using namespace Configs;

		glDisable(GL_CULL_FACE);

		GLuint currentProgram = depthProgram;

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);

		glViewport(0, 0, 1024, 1024);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(currentProgram);

		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
		glm::mat4 depthViewMatrix = glm::lookAt(LightCenter, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		// or, for spot light :
		/*glm::vec3 lightPos(5, 20, 20);
		glm::mat4 depthProjectionMatrix = glm::perspective<float>(45.0f, 1.0f, 2.0f, 50.0f);
		glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos-LightCenter, glm::vec3(0,1,0));*/

		glm::mat4 depthModelMatrix = glm::mat4(1.0);

		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "MVP"), 1, GL_FALSE, &depthMVP[0][0]);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,  // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glEnable(GL_CULL_FACE);
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
		glUseProgram(NULL);

		glViewport(0, 0, 512, 512);
	}

	void RenderWithShadowMap16(GLuint FrameBuffer)
	{
		using namespace Configs;
		//glDisable(GL_CULL_FACE);
		// Render to the FrameBuffer
		glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);

		RenderLightPositionAlter();

		GLuint currentProgram = shadowProgram;

		glViewport(0, 0, parameters.screenWidth, parameters.screenHeight);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

							 // Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(currentProgram);

		// Compute the MVP matrix from keyboard and mouse input
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);

		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
		glm::mat4 depthViewMatrix = glm::lookAt(LightCenter, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0);
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

		GLuint ProjectionMatrixID = glGetUniformLocation(currentProgram, "proj");
		GLuint ViewMatrixID = glGetUniformLocation(currentProgram, "view");
		GLuint ModelMatrixID = glGetUniformLocation(currentProgram, "model");
		GLuint dProjectionMatrixID = glGetUniformLocation(currentProgram, "proj_depth");
		GLuint dViewMatrixID = glGetUniformLocation(currentProgram, "view_depth");
		GLuint ShadowMapID = glGetUniformLocation(currentProgram, "shadowMap");
		GLuint debugColorID = glGetUniformLocation(currentProgram, "debugColor");
		GLuint biasID = glGetUniformLocation(currentProgram, "bias");
		GLuint lightInvDirID = glGetUniformLocation(currentProgram, "LightInvDirection_worldspace");

		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(ProjectionMatrixID, 1, GL_FALSE, &ProjectionMatrix[0][0]);
		glUniformMatrix4fv(dViewMatrixID, 1, GL_FALSE, &depthViewMatrix[0][0]);
		glUniformMatrix4fv(dProjectionMatrixID, 1, GL_FALSE, &depthProjectionMatrix[0][0]);
		glUniform3f(debugColorID, 1, 1, 1);
		glUniform3f(lightInvDirID, LightCenter.x, LightCenter.y, LightCenter.z);
		glUniform1f(biasID, shadowBias);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 0);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		//glEnable(GL_CULL_FACE);
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void RenderLightPosition(GLuint FrameBuffer)
	{
		RenderLightPosition(FrameBuffer, 0.5);
	}

	void RenderLightPosition(GLuint FrameBuffer, float lightScale)
	{
		using namespace Configs;

		glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);

		GLuint currentProgram = debugProgram;
		glUseProgram(currentProgram);

		GLuint vertexPositionLocation = glGetAttribLocation(currentProgram, "position");

		glm::mat4 view = getViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "view"), 1, GL_FALSE, &view[0][0]);

		glm::mat4 proj = getProjectionMatrix();
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "proj"), 1, GL_FALSE, &proj[0][0]);

		/*
		*	Draw the Light Rect
		*/
		glm::mat4 model(1.0f);
		// Light Center = vec3(0, 6, 32);
		model = glm::translate(model, LightCenter);

		/*
		vec3 rotation_y(vec3 v, float a){return vec3(v.x*cos(a) + v.z*sin(a), v.y, -v.x*sin(a) + v.z*cos(a));}
		vec3 rotation_z(vec3 v, float a){return vec3(v.x*cos(a) - v.y*sin(a), v.x*sin(a) + v.y*cos(a), v.z);}
		vec3 rotation_yz(vec3 v, float ay, float az){return rotation_z(rotation_y(v, ay), az);}
		*/

		/*
		glm::vec3 v = glm::vec3(0, 1, 0);
		v = glm::vec3(v.x*cos(roty) + v.z*sin(roty), v.y, -v.x*sin(roty) + v.z*cos(roty));
		glm::vec3 rotAxisAfterYRotated = glm::vec3(v.x*cos(roty) - v.y*sin(roty), v.x*sin(roty) + v.y*cos(roty), v.z);
		model = glm::rotate(model, -roty * 2.0f * 3.14f, v);

		v = glm::vec3(0, 0, 1);
		v = glm::vec3(v.x*cos(rotz) - v.y*sin(rotz), v.x*sin(rotz) + v.y*cos(rotz), v.z);
		*/

		model = glm::rotate(model, roty * 2.0f * 3.14f, glm::vec3(0, 1, 0));
		//model = glm::rotate(model, -rotz * 2.0f * 3.14f, glm::vec3(0, 0, 1));
		model = glm::scale(model, glm::vec3(width * lightScale, height * lightScale, 1));

		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "model"), 1, GL_FALSE, &model[0][0]);
		glUniform3f(glGetUniformLocation(currentProgram, "debugColor"), 1.0, 1.0, 1.0);
		//glUniform1f(glGetUniformLocation(currentProgram, "Intensity"), lightIntensity);

		glBindBuffer(GL_ARRAY_BUFFER, lightRectBuffer);
		glVertexAttribPointer(vertexPositionLocation, 3, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(vertexPositionLocation);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(vertexPositionLocation);

		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void RenderLightPositionAlter()
	{
		GLuint currentProgram = Configs::debugProgram;
		glUseProgram(currentProgram);

		GLuint vertexPositionLocation = glGetAttribLocation(currentProgram, "position");

		glm::mat4 view = getViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "view"), 1, GL_FALSE, &view[0][0]);

		glm::mat4 proj = getProjectionMatrix();
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "proj"), 1, GL_FALSE, &proj[0][0]);

		glm::mat4 model(1.0f);
		model = glm::translate(model, Configs::LightCenter);

		model = glm::rotate(model, Configs::roty * 2.0f * 3.14f, glm::vec3(0, 1, 0));
		model = glm::scale(model, glm::vec3(Configs::width * 0.5, Configs::height * 0.5, 1));

		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "model"), 1, GL_FALSE, &model[0][0]);
		glUniform3f(glGetUniformLocation(currentProgram, "debugColor"), 1.0, 1.0, 1.0);
		//glUniform1f(glGetUniformLocation(currentProgram, "Intensity"), Configs::lightIntensity);

		glBindBuffer(GL_ARRAY_BUFFER, Configs::lightRectBuffer);
		glVertexAttribPointer(vertexPositionLocation, 3, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(vertexPositionLocation);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(vertexPositionLocation);

		//glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void RenderSceneGeometryAlter(GLuint FrameBuffer)
	{
		using namespace Configs;

		glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/*
		*	Draw Light Rect
		*/
		RenderLightPositionAlter();

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		
		/*
		*	Draw Scene
		*/
		GLuint currentProgram = bgfxProgram;

		glUseProgram(currentProgram);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ltc_mat_texture);
		glUniform1i(glGetUniformLocation(currentProgram, "ltc_mat"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ltc_mag_texture);
		glUniform1i(glGetUniformLocation(currentProgram, "ltc_mag"), 1);

		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);

		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "model"), 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniform3f(glGetUniformLocation(currentProgram, "LightCenter"), LightCenter.x, LightCenter.y, LightCenter.z);
		glUniform1f(glGetUniformLocation(currentProgram, "roughness"), roughness);
		glUniform3f(glGetUniformLocation(currentProgram, "dcolor"), dsColor.dcolor[0], dsColor.dcolor[1], dsColor.dcolor[2]);
		glUniform3f(glGetUniformLocation(currentProgram, "scolor"), dsColor.scolor[0], dsColor.scolor[1], dsColor.scolor[2]);
		glUniform1f(glGetUniformLocation(currentProgram, "intensity"), lightIntensity);
		glUniform1f(glGetUniformLocation(currentProgram, "width"), width);
		glUniform1f(glGetUniformLocation(currentProgram, "height"), height);
		glUniform1f(glGetUniformLocation(currentProgram, "roty"), roty);
		glUniform1f(glGetUniformLocation(currentProgram, "rotz"), rotz);
		glUniform1i(glGetUniformLocation(currentProgram, "twoSided"), twoSided);
		glUniform1i(glGetUniformLocation(currentProgram, "mode"), mode);
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "view"), 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniform2f(glGetUniformLocation(currentProgram, "resolution"), parameters.screenWidth, parameters.screenHeight);
		glUniform3f(glGetUniformLocation(currentProgram, "u_viewPosition"), cameraEyePos.x, cameraEyePos.y, cameraEyePos.z);


		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);


		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);


		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void RenderSceneGeometryAlterAlter(GLuint FrameBuffer)
	{
		using namespace Configs;

		glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		/*
		*	Draw Scene
		*/
		GLuint currentProgram = bgfxShadowProgram;

		glUseProgram(currentProgram);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ltc_mat_texture);
		glUniform1i(glGetUniformLocation(currentProgram, "ltc_mat"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ltc_mag_texture);
		glUniform1i(glGetUniformLocation(currentProgram, "ltc_mag"), 1);

		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);

		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
		glm::mat4 depthViewMatrix = glm::lookAt(LightCenter, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

		GLuint dProjectionMatrixID = glGetUniformLocation(currentProgram, "proj_depth");
		GLuint dViewMatrixID = glGetUniformLocation(currentProgram, "view_depth");
		GLuint ShadowMapID = glGetUniformLocation(currentProgram, "shadowMap");
		GLuint biasID = glGetUniformLocation(currentProgram, "bias");

		glUniformMatrix4fv(dViewMatrixID, 1, GL_FALSE, &depthViewMatrix[0][0]);
		glUniformMatrix4fv(dProjectionMatrixID, 1, GL_FALSE, &depthProjectionMatrix[0][0]);
		glUniform1f(biasID, shadowBias);
		
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(ShadowMapID, 2);

		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "model"), 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniform3f(glGetUniformLocation(currentProgram, "LightCenter"), LightCenter.x, LightCenter.y, LightCenter.z);
		glUniform1f(glGetUniformLocation(currentProgram, "roughness"), roughness);
		glUniform3f(glGetUniformLocation(currentProgram, "dcolor"), dsColor.dcolor[0], dsColor.dcolor[1], dsColor.dcolor[2]);
		glUniform3f(glGetUniformLocation(currentProgram, "scolor"), dsColor.scolor[0], dsColor.scolor[1], dsColor.scolor[2]);
		glUniform1f(glGetUniformLocation(currentProgram, "intensity"), lightIntensity);
		glUniform1f(glGetUniformLocation(currentProgram, "width"), width);
		glUniform1f(glGetUniformLocation(currentProgram, "height"), height);
		glUniform1f(glGetUniformLocation(currentProgram, "roty"), roty);
		glUniform1f(glGetUniformLocation(currentProgram, "rotz"), rotz);
		glUniform1i(glGetUniformLocation(currentProgram, "twoSided"), twoSided);
		glUniform1i(glGetUniformLocation(currentProgram, "mode"), mode);
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "view"), 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniform2f(glGetUniformLocation(currentProgram, "resolution"), parameters.screenWidth, parameters.screenHeight);
		glUniform3f(glGetUniformLocation(currentProgram, "u_viewPosition"), cameraEyePos.x, cameraEyePos.y, cameraEyePos.z);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);


		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);


		// Index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// Draw the triangles !
		glDrawElements(
			GL_TRIANGLES,      // mode
			indices.size(),    // count
			GL_UNSIGNED_SHORT, // type
			(void*)0           // element array buffer offset
		);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		/*
		*	Draw Light Rect
		*/
		RenderLightPositionAlter();
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void BlendShadowMask(GLuint ShadingTexture, GLuint ShadowMaskTexture)
	{
		using namespace Configs;

		GLuint currentProgram = shadowMaskProgram;

		glUseProgram(currentProgram);
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glDisable(GL_BLEND);

		/* line 633 */
		// Set textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ShadingTexture);
		glUniform1i(glGetUniformLocation(currentProgram, "MainTex"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ShadowMaskTexture);
		glUniform1i(glGetUniformLocation(currentProgram, "shadowMaskTex"), 1);
		glUniform1f(glGetUniformLocation(currentProgram, "Intensity"), lightIntensity);

		/* line 638 */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glUniform2f(glGetUniformLocation(currentProgram, "resolution"), parameters.screenWidth, parameters.screenHeight);

		/* line 645 */
		// Blit pass
		glBindBuffer(GL_ARRAY_BUFFER, screenBuffer);
		GLuint vertexPositionLocation = glGetAttribLocation(currentProgram, "position");
		glEnableVertexAttribArray(vertexPositionLocation);
		glVertexAttribPointer(vertexPositionLocation, 2, GL_FLOAT, false, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisableVertexAttribArray(vertexPositionLocation);
	}

	void RenderScene()
	{
		using namespace Configs;

		GLuint currentProgram = ltcProgram;

		// Load program into GPU
		glUseProgram(currentProgram);
		/* line - 566*/

		glMatrixMode(GL_MODELVIEW);
		/* line 571*/
		glLoadIdentity();
		glPushMatrix();
		glTranslated(0.0, 3.0, eyez);
		GLfloat view[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, view);
		glPopMatrix();

		glUniform1f(glGetUniformLocation(currentProgram, "roty"), roty);
		glUniform1f(glGetUniformLocation(currentProgram, "rotz"), rotz);
		glUniform1f(glGetUniformLocation(currentProgram, "height"), height);
		glUniform1f(glGetUniformLocation(currentProgram, "width"), width);
		glUniform1f(glGetUniformLocation(currentProgram, "intensity"), lightIntensity);
		glUniform1f(glGetUniformLocation(currentProgram, "roughness"), roughness);
		glUniform1i(glGetUniformLocation(currentProgram, "twoSided"), twoSided);
		glUniform3f(glGetUniformLocation(currentProgram, "dcolor"), dsColor.dcolor[0], dsColor.dcolor[1], dsColor.dcolor[2]);
		glUniform3f(glGetUniformLocation(currentProgram, "scolor"), dsColor.scolor[0], dsColor.scolor[1], dsColor.scolor[2]);
		glUniform1i(glGetUniformLocation(currentProgram, "mode"), mode);

		/* line 594 - 596*/
		glUniformMatrix4fv(glGetUniformLocation(currentProgram, "view"), 1, GL_FALSE, &(getViewMatrix())[0][0]);
		glUniform2f(glGetUniformLocation(currentProgram, "resolution"), parameters.screenWidth, parameters.screenHeight);

		/* line 598 - */
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ltc_mat_texture);
		glUniform1i(glGetUniformLocation(currentProgram, "ltc_mat"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, ltc_mag_texture);
		glUniform1i(glGetUniformLocation(currentProgram, "ltc_mag"), 1);
		/* line - 604 */

		glUniform3f(glGetUniformLocation(currentProgram, "LightCenter"), LightCenter.x, LightCenter.y, LightCenter.z);

		/* line 608 -  */
		glBindFramebuffer(GL_FRAMEBUFFER, rttFramebuffer);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		/* line - 619 */

		/* line 621 - */
		// Render geometry
		GLuint vertexPositionLocation = glGetAttribLocation(currentProgram, "position");
		glBindBuffer(GL_ARRAY_BUFFER, screenBuffer);
		glVertexAttribPointer(vertexPositionLocation, 2, GL_FLOAT, false, 0, 0);
		glEnableVertexAttribArray(vertexPositionLocation);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(vertexPositionLocation);

		/* line 628 */
		glBindFramebuffer(GL_FRAMEBUFFER, NULL);
	}

	void BiltRender()
	{
		using namespace Configs;

		GLuint currentProgram = blitProgram;

		glUseProgram(currentProgram);
		glDisable(GL_BLEND);

		GLuint tex = rttTexture;

		/* line 633 */
		// Set textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex);
		glUniform1i(glGetUniformLocation(currentProgram, "tex"), 0);

		/* line 638 */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glUniform2f(glGetUniformLocation(currentProgram, "resolution"), parameters.screenWidth, parameters.screenHeight);

		/* line 645 */
		// Blit pass
		GLuint vertexPositionLocation = glGetAttribLocation(currentProgram, "position");
		glBindBuffer(GL_ARRAY_BUFFER, screenBuffer);
		glEnableVertexAttribArray(vertexPositionLocation);
		glVertexAttribPointer(vertexPositionLocation, 2, GL_FLOAT, false, 0, 0);

		glDrawArrays(GL_TRIANGLES, 0, 6);

		return;
	}
};
#endif // !T16