#include "pch.h"
#include "MyGUI_OpenGLESRenderManager.h"
#include "MyGUI_OpenGLESTexture.h"
#include "MyGUI_OpenGLESVertexBuffer.h"
#include "MyGUI_OpenGLESDiagnostic.h"
#include "MyGUI_VertexData.h"
#include "MyGUI_Gui.h"
#include "MyGUI_Timer.h"

#include <GLES2/gl2ext.h>
#include "platform.h"

const char* vShader = " \n\
\n\
attribute vec3 a_position;                             \n\
attribute vec4 a_color;                                \n\
attribute vec2 a_texCoord;                             \n\
uniform        mat4 u_MVPMatrix;                       \n\
\n\
varying lowp vec4 v_fragmentColor;                     \n\
varying mediump vec2 v_texCoord;                       \n\
\n\
void main()                                            \n\
{                                                      \n\
gl_Position = (vec4(a_position,1));                    \n\
v_fragmentColor = a_color;                             \n\
v_texCoord = a_texCoord;                               \n\
}                                                      \n\
";

const char* fShader = " \n\
precision lowp float;                                  \n\
varying vec4 v_fragmentColor;                          \n\
varying vec2 v_texCoord;                               \n\
uniform sampler2D u_texture;                           \n\
void main(void) {                                      \n\
    gl_FragColor = texture2D(u_texture, v_texCoord);   \n\
}                                                      \n\
";

// attribute index
enum {
    ATTRIB_VERTEX,
    ATTRIB_COLOR,
    NUM_ATTRIBUTES
};

namespace MyGUI
{

	OpenGLESRenderManager::OpenGLESRenderManager() :
		mUpdate(false),
		mImageLoader(nullptr),
		mPboIsSupported(false),
		mIsInitialise(false)
	{
	}

	void OpenGLESRenderManager::initialise(OpenGLESImageLoader* _loader)
	{
		MYGUI_PLATFORM_ASSERT(!mIsInitialise, getClassTypeName() << " initialised twice");
		MYGUI_PLATFORM_LOG(Info, "* Initialise: " << getClassTypeName());

		mVertexFormat = VertexColourType::ColourABGR; // WDY ??? ColourABGR

		mUpdate = false;
		mImageLoader = _loader;

		//glewInit();

		//mPboIsSupported = glewIsExtensionSupported("GL_EXT_pixel_buffer_object") != 0;
        
        mProgram = BuildProgram(vShader, fShader);
        
        //MYGUI_PLATFORM_ASSERT(r, "ShaderProgram Initialise fails! ");

        //p->updateUniforms();
        
        
        /*
        _positionSlot = glGetAttribLocation(mShaderProgram->getProgram(), "a_position");
        CHECK_GL_ERROR_DEBUG();
        _colorSlot = glGetAttribLocation(mShaderProgram->getProgram(), "a_color");
        CHECK_GL_ERROR_DEBUG();
        _texSlot = glGetAttribLocation(mShaderProgram->getProgram(), "a_texCoord");
        CHECK_GL_ERROR_DEBUG();
         */

		MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully initialized");
		mIsInitialise = true;
	}

	void OpenGLESRenderManager::shutdown()
	{
		MYGUI_PLATFORM_ASSERT(mIsInitialise, getClassTypeName() << " is not initialised");
		MYGUI_PLATFORM_LOG(Info, "* Shutdown: " << getClassTypeName());

		destroyAllResources();

		MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully shutdown");
		mIsInitialise = false;
	}

	IVertexBuffer* OpenGLESRenderManager::createVertexBuffer()
	{
		return new OpenGLESVertexBuffer();
	}

	void OpenGLESRenderManager::destroyVertexBuffer(IVertexBuffer* _buffer)
	{
		delete _buffer;
	}
    
    
    GLuint OpenGLESRenderManager::BuildShader(const char* source, GLenum shaderType) const
    {
        GLuint shaderHandle = glCreateShader(shaderType);
        glShaderSource(shaderHandle, 1, &source, 0);
        glCompileShader(shaderHandle);
        CHECK_GL_ERROR_DEBUG();
        
        GLint compileSuccess;
        glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
        
        if (compileSuccess == GL_FALSE) {
            GLchar messages[256];
            glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]);
            MYGUI_PLATFORM_EXCEPT(messages);
        }
        
        return shaderHandle;
    }
    
    GLuint OpenGLESRenderManager::BuildProgram(const char* vertexShaderSource,
                                          const char* fragmentShaderSource) const
    {
        GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
        GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
        
        GLuint programHandle = glCreateProgram();
        glAttachShader(programHandle, vertexShader);
        glAttachShader(programHandle, fragmentShader);
        glLinkProgram(programHandle);
        CHECK_GL_ERROR_DEBUG();
        
        GLint linkSuccess;
        glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
        if (linkSuccess == GL_FALSE) {
            GLchar messages[256];
            glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]);
            MYGUI_PLATFORM_EXCEPT(messages);
        }
        
        return programHandle;
    }

	void OpenGLESRenderManager::doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count)
	{
		OpenGLESVertexBuffer* buffer = static_cast<OpenGLESVertexBuffer*>(_buffer);
		unsigned int buffer_id = buffer->getBufferID();
		MYGUI_PLATFORM_ASSERT(buffer_id, "Vertex buffer is not created");

		unsigned int texture_id = 0;
		if (_texture)
		{
			OpenGLESTexture* texture = static_cast<OpenGLESTexture*>(_texture);
			texture_id = texture->getTextureID();
			//MYGUI_PLATFORM_ASSERT(texture_id, "Texture is not created");
		}
        
        

		glBindTexture(GL_TEXTURE_2D, texture_id);
        CHECK_GL_ERROR_DEBUG();

		//glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer_id);
        glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
        CHECK_GL_ERROR_DEBUG();

        
        GLuint positionSlot = glGetAttribLocation(mProgram, "a_position");
        GLuint colorSlot = glGetAttribLocation(mProgram, "a_color");
        GLuint texSlot = glGetAttribLocation(mProgram, "a_texCoord");
        
        GLuint textureUniform = glGetUniformLocation(mProgram, "u_texture");
        
        
        glEnableVertexAttribArray(positionSlot);
        glEnableVertexAttribArray(colorSlot);
        glEnableVertexAttribArray(texSlot);
        
        glUseProgram(mProgram);
        
        /*
        _positionSlot = glGetAttribLocation(mShaderProgram->getProgram(), "a_position");
        _colorSlot = glGetAttribLocation(mShaderProgram->getProgram(), "a_color");
        _texSlot = glGetAttribLocation(mShaderProgram->getProgram(), "a_texCoord");
        
        glEnableVertexAttribArray(_positionSlot);
        glEnableVertexAttribArray(_colorSlot);
        glEnableVertexAttribArray(_texSlot);
        */
        
        //mShaderProgram->setUniformForModelViewProjectionMatrix();
        
        //ccGLBindTexture2D( m_uName );
        
        #define kQuadSize sizeof(Vertex)
        
        size_t offset = 0;
        int diff = offsetof( Vertex, x);
        glVertexAttribPointer(positionSlot, 3, GL_FLOAT, GL_FALSE, kQuadSize, (void*) (offset + diff));
        CHECK_GL_ERROR_DEBUG();
        
        diff = offsetof( Vertex, colour);
        glVertexAttribPointer(colorSlot, 4, GL_UNSIGNED_BYTE, GL_TRUE, kQuadSize, (void*) (offset + diff));
        CHECK_GL_ERROR_DEBUG();
        
        diff = offsetof( Vertex, u);
        glVertexAttribPointer(texSlot, 2, GL_FLOAT, GL_FALSE, kQuadSize, (void*) (offset + diff));
        CHECK_GL_ERROR_DEBUG();
        
        glUniform1i(textureUniform, 0);
        
        //glDrawElements(GL_TRIANGLES, _count, GL_UNSIGNED_BYTE, 0);
        glDrawArrays(GL_TRIANGLES, 0, _count);
        //CHECK_GL_ERROR_DEBUG();
        do {
            GLenum __error = glGetError();
            if(__error) {
                MYGUI_PLATFORM_LOG(Info, "OpenGLES error 0x%04X in " << __error << " " << __FILE__<< " " << __FUNCTION__<< " "<< __LINE__);
                MYGUI_PLATFORM_LOG(Error, "texture_id:"<<texture_id << ",buffer_id:" << buffer_id);
            }
        } while (false);
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        CHECK_GL_ERROR_DEBUG();
		glBindTexture(GL_TEXTURE_2D, 0);
        CHECK_GL_ERROR_DEBUG();
	}

	void OpenGLESRenderManager::begin()
	{
        CHECK_GL_ERROR_DEBUG();
        glClearColor(0.8, 0.8, 0.8, 1);
        CHECK_GL_ERROR_DEBUG();
        glClear(GL_COLOR_BUFFER_BIT);
        CHECK_GL_ERROR_DEBUG();
		glDisable(GL_DEPTH_TEST);
        CHECK_GL_ERROR_DEBUG();
        glEnable(GL_BLEND);
        CHECK_GL_ERROR_DEBUG();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        CHECK_GL_ERROR_DEBUG();

	}

	void OpenGLESRenderManager::end()
	{
        
	}

	const RenderTargetInfo& OpenGLESRenderManager::getInfo()
	{
		return mInfo;
	}

	const IntSize& OpenGLESRenderManager::getViewSize() const
	{
		return mViewSize;
	}

	VertexColourType OpenGLESRenderManager::getVertexFormat()
	{
		return mVertexFormat;
	}
    
    void OpenGLESRenderManager::renderScene(void)
    {

    }

	void OpenGLESRenderManager::drawOneFrame()
	{
		Gui* gui = Gui::getInstancePtr();
		if (gui == nullptr)
			return;

		static Timer timer;
		static unsigned long last_time = timer.getMilliseconds();
		unsigned long now_time = timer.getMilliseconds();
		unsigned long time = now_time - last_time;

		onFrameEvent((float)((double)(time) / (double)1000));

		last_time = now_time;

		begin();
        //renderScene();
		onRenderToTarget(this, mUpdate);
		end();
        angle++;
        //step++;

		mUpdate = false;
	}

	void OpenGLESRenderManager::setViewSize(int _width, int _height)
	{
		if (_height == 0)
			_height = 1;
		if (_width == 0)
			_width = 1;

		mViewSize.set(_width, _height);

		mInfo.maximumDepth = 1;
		mInfo.hOffset = 0;
		mInfo.vOffset = 0;
		mInfo.aspectCoef = float(mViewSize.height) / float(mViewSize.width);
		mInfo.pixScaleX = 1.0f / float(mViewSize.width);
		mInfo.pixScaleY = 1.0f / float(mViewSize.height);

		onResizeView(mViewSize);
		mUpdate = true;
	}

	bool OpenGLESRenderManager::isPixelBufferObjectSupported() const
	{
		return mPboIsSupported;
	}

	ITexture* OpenGLESRenderManager::createTexture(const std::string& _name)
	{
		MapTexture::const_iterator item = mTextures.find(_name);
		MYGUI_PLATFORM_ASSERT(item == mTextures.end(), "Texture '" << _name << "' already exist");

		OpenGLESTexture* texture = new OpenGLESTexture(_name, mImageLoader);
		mTextures[_name] = texture;
		return texture;
	}

	void OpenGLESRenderManager::destroyTexture(ITexture* _texture)
	{
		if (_texture == nullptr)
			return;

		MapTexture::iterator item = mTextures.find(_texture->getName());
		MYGUI_PLATFORM_ASSERT(item != mTextures.end(), "Texture '" << _texture->getName() << "' not found");

		mTextures.erase(item);
		delete _texture;
	}

	ITexture* OpenGLESRenderManager::getTexture(const std::string& _name)
	{
		MapTexture::const_iterator item = mTextures.find(_name);
		if (item == mTextures.end())
			return nullptr;
		return item->second;
	}

	void OpenGLESRenderManager::destroyAllResources()
	{
		for (MapTexture::const_iterator item = mTextures.begin(); item != mTextures.end(); ++item)
		{
			delete item->second;
		}
		mTextures.clear();
	}


} // namespace MyGUI
