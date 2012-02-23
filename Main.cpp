#include "Framework.h"
#include "Shader.h"
#include <assert.h>
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"
#include <vector>

//TODO
#define MODEL_PATH "/Users/grantm/class/cs248/assign3/models/teapot.3ds"
#define SPEC_SUFFIX "_s.jpg"
#define NORM_SUFFIX "_n.jpg"
#define DIFF_SUFFIX "_d.jpg"

using namespace std;

// Note: See the SMFL documentation for info on setting up fullscreen mode
// and using rendering settings
// http://www.sfml-dev.org/tutorials/1.6/window-window.php
sf::WindowSettings settings(24, 8, 2);
sf::Window window(sf::VideoMode(800, 600), "CS248 Rules!", sf::Style::Close, settings);

// This is a clock you can use to control animation.  For more info, see:
// http://www.sfml-dev.org/tutorials/1.6/window-time.php
sf::Clock clck;

// This creates an asset importer using the Open Asset Import library.
// It automatically manages resources for you, and frees them when the program
// exits.
Assimp::Importer importer;

void initOpenGL();
void loadAssets();
void handleInput();
void renderFrame();

#define GL_CHECK(x) {\
(x);\
GLenum error = glGetError();\
if (GL_NO_ERROR != error) {\
    printf("%s", gluErrorString(error));\
}\
}


// ----------------------------------------------------------------------------
void color4_to_float4(const struct aiColor4D *c, float f[4])
{
    f[0] = c->r;
    f[1] = c->g;
    f[2] = c->b;
    f[3] = c->a;
}

// ----------------------------------------------------------------------------
void set_float4(float f[4], float a, float b, float c, float d)
{
    f[0] = a;
    f[1] = b;
    f[2] = c;
    f[3] = d;
}

void apply_material(const struct aiMaterial *mtl)
{
    float c[4];

    GLenum fill_mode;
    int ret1, ret2;
    struct aiColor4D diffuse;
    struct aiColor4D specular;
    struct aiColor4D ambient;
    struct aiColor4D emission;
    float shininess, strength;
    int two_sided;
    int wireframe;
    unsigned int max;

    set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
            color4_to_float4(&diffuse, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
            color4_to_float4(&specular, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

    set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
            color4_to_float4(&ambient, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
            color4_to_float4(&emission, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

    max = 1;
    ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
    if(ret1 == AI_SUCCESS) {
    max = 1;
    ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
            if(ret2 == AI_SUCCESS)
                    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
    else
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    }
    else {
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
            set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
    }

    max = 1;
    if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
            fill_mode = wireframe ? GL_LINE : GL_FILL;
    else
            fill_mode = GL_FILL;
    glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

    max = 1;
    if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
            glDisable(GL_CULL_FACE);
    else
            glEnable(GL_CULL_FACE);
}


int main(int argc, char** argv) {

    initOpenGL();
    loadAssets();

    // Put your game loop here (i.e., render with OpenGL, update animation)
    while (window.IsOpened()) {
        handleInput();
        renderFrame();
        window.Display();
    }

    return 0;
}



void initOpenGL() {
    // Initialize GLEW on Windows, to make sure that OpenGL 2.0 is loaded
#ifdef FRAMEWORK_USE_GLEW
    GLint error = glewInit();
    if (GLEW_OK != error) {
        std::cerr << glewGetErrorString(error) << std::endl;
        exit(-1);
    }
    if (!GLEW_VERSION_2_0 || !GL_EXT_framebuffer_object) {
        std::cerr << "This program requires OpenGL 2.0 and FBOs" << std::endl;
        exit(-1);
    }
#endif

    // This initializes OpenGL with some common defaults.  More info here:
    // http://www.sfml-dev.org/tutorials/1.6/window-opengl.php
    glClearDepth(1.0f);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, window.GetWidth(), window.GetHeight());
}

struct asset {
    const aiScene* scene;
    sf::Image diff_texture;
    sf::Image spec_texture;
};

vector<asset> assets;

asset loadAsset(const char* name) {
    asset a;

    a.scene = importer.ReadFile(name,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcessPreset_TargetRealtime_Quality);

    if (!a.scene || a.scene->mNumMeshes <= 0) {
        std::cerr << importer.GetErrorString() << std::endl;
        exit(-1);
    }

    cout << "GO" << endl;

    if (a.scene->HasMaterials()) {
        cout << "..." << endl;
        aiString path;
        for (int j = 0; j < a.scene->mNumMaterials; ++j) {
            for (int i = 0; i < a.scene->mMaterials[j]->mNumAllocated; ++i) {
                cout << i << endl;
                a.scene->mMaterials[j]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
                cout << path.data << endl;
            }
        }
    }

    return a;
}

void loadAssets() {
    asset a = loadAsset(MODEL_PATH);
    assets.push_back(a);

    // Read in an asset file, and do some post-processing.  There is much
    // more you can do with this asset loader, including load textures.
    // More info is here:
    // http://assimp.sourceforge.net/lib_html/usage.html

    //////////////////////////////////////////////////////////////////////////
    // TODO: LOAD YOUR SHADERS/TEXTURES
    //////////////////////////////////////////////////////////////////////////
}


float dx = 0, dy = 0;
float x = 0, y = 0;

void handleInput() {
    //////////////////////////////////////////////////////////////////////////
    // TODO: ADD YOUR INPUT HANDLING HERE.
    //////////////////////////////////////////////////////////////////////////

    // Event loop, for processing user input, etc.  For more info, see:
    // http://www.sfml-dev.org/tutorials/1.6/window-events.php
    sf::Event evt;
    while (window.GetEvent(evt)) {
        switch (evt.Type) {
            case sf::Event::Closed:
                // Close the window.  This will cause the game loop to exit,
                // because the IsOpened() function will no longer return true.
                window.Close();
                break;
            case sf::Event::KeyPressed:
                if (evt.Key.Code == sf::Key::W) dy -= .1;
                if (evt.Key.Code == sf::Key::S) dy += .1;

                if (evt.Key.Code == sf::Key::A) dx -= .1;
                if (evt.Key.Code == sf::Key::D) dx += .1;

            case sf::Event::Resized:
                // If the window is resized, then we need to change the perspective
                // transformation and viewport
                glViewport(0, 0, evt.Size.Width, evt.Size.Height);
                break;
            default:
                break;
        }
    }
}


void recursive_render (const struct aiScene *sc, struct aiNode *nd) {
    struct aiMatrix4x4 m = nd->mTransformation;
    m.Transpose();
    glPushMatrix();
    glMultMatrixf((float*)&m);
    //glTranslatef(dx, dy, 0);

    for (int n = 0; n < nd->mNumMeshes; ++n) {
        const struct aiMesh* mesh = sc->mMeshes[nd->mMeshes[n]];

        //TODO: For now.
        glEnable(GL_LIGHTING);

        for (int t = 0; t < mesh->mNumFaces; ++t) {
            const struct aiFace* face = &mesh->mFaces[t];

            apply_material(sc->mMaterials[mesh->mMaterialIndex]);

            GLenum face_mode;

            switch (face->mNumIndices) {
                case 1:  face_mode = GL_POINTS;    break;
                case 2:  face_mode = GL_LINES;     break;
                case 3:  face_mode = GL_TRIANGLES; break;
                default: face_mode = GL_POLYGON;   break;
            }

            glBegin(face_mode);
            for (int i = 0; i < face->mNumIndices; ++i) {
                int index = face->mIndices[i];

                if (mesh->mColors[0] != NULL) glColor4fv((GLfloat*) &mesh->mColors[0][index]);
                if (mesh->mNormals   != NULL) glNormal3fv(&mesh->mNormals[index].x);
                glVertex3fv(&mesh->mVertices[index].x);
            }
            glEnd();
        }
    }

    for (int n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n]);
    }

    glPopMatrix();
}


void renderFrame() {
    //////////////////////////////////////////////////////////////////////////
    // TODO: ADD YOUR RENDERING CODE HERE.  You may use as many .cpp files
    // in this assignment as you wish.
    //////////////////////////////////////////////////////////////////////////
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport(0, 0, window.GetWidth(), window.GetHeight());

    // start fresh
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();


    if (dx != 0 || dy != 0) {
        x += dx;
        y += dy;
        dx = 0;
        dy = 0;
    }
    glTranslatef(x, y, 0);

    cout << "RENDER" << endl;

    for (int i = 0; i < assets.size(); ++i) {
        recursive_render(assets[i].scene, assets[i].scene->mRootNode);
    }
}






