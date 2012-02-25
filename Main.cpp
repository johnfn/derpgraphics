#include "Framework.h"
#include "Shader.h"
#include <assert.h>
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"
#include <vector>
#include <fstream>

//TODO
#define BASE_PATH "/Users/grantm/class/cs248/assign3/models/"
#define MODEL_PATH "/Users/grantm/class/cs248/assign3/models/cathedral.3ds"
#define SPEC_SUFFIX "_s.jpg"
#define NORM_SUFFIX "_n.jpg"
#define DIFF_SUFFIX "_d.jpg"

using namespace std;

// Note: See the SMFL documentation for info on setting up fullscreen mode
// and using rendering settings
// http://www.sfml-dev.org/tutorials/1.6/window-window.php
sf::WindowSettings settings(24, 8, 2);
sf::Window window(sf::VideoMode(800, 600), "CS248 Rules!", sf::Style::Close, settings);

std::map<pair<string, aiTextureType>, sf::Image*> textureIdMap; //TODO: Not an ID map any more.

// This is a clock you can use to control animation.  For more info, see:
// http://www.sfml-dev.org/tutorials/1.6/window-time.php
sf::Clock clck;

std::auto_ptr<Shader> shader;

std::auto_ptr<sf::Image> diff_text;
std::auto_ptr<sf::Image> spec_text;

// This creates an asset importer using the Open Asset Import library.
// It automatically manages resources for you, and frees them when the program
// exits.
Assimp::Importer importer;

void initOpenGL();
void loadAssets();
void handleInput();
void renderFrame();
void drawMesh(const struct aiMesh *mesh);
void setMaterial(const struct aiScene *scene, const struct aiMesh *mesh);
void setTextures();
void LoadGLTextures(const aiScene* scene);

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
    LoadGLTextures(a.scene); //TODO move into loadassets

    return a;
}

void loadAssets() {
    asset a = loadAsset(MODEL_PATH);
    assets.push_back(a);

    // Read in an asset file, and do some post-processing.  There is much
    // more you can do with this asset loader, including load textures.
    // More info is here:
    // http://assimp.sourceforge.net/lib_html/usage.html

    shader.reset(new Shader("shaders/phong"));

    if (!shader->loaded()) {
        cerr << "Shader failed to load." << endl;
        exit(-1);
    }

    /*
    */

    //loadShader(fragShader, "/Users/grantm/class/cs248/assign3/shaders/phong.frag.glsl");
    //loadShader(vertShader, "/Users/grantm/class/cs248/assign3/shaders/phong.vert.glsl");


    //////////////////////////////////////////////////////////////////////////
    // TODO: LOAD YOUR SHADERS/TEXTURES
    //////////////////////////////////////////////////////////////////////////
}


float dx = 0, dy = 0;
float x = 0, y = 0;
float rx = 0, ry = 0, drx = 0, dry = 0;

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

                if (evt.Key.Code == sf::Key::Up)   drx += 1;
                if (evt.Key.Code == sf::Key::Down) drx -= 1;

                if (evt.Key.Code == sf::Key::Left)   dry += 1;
                if (evt.Key.Code == sf::Key::Right)  dry -= 1;


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

        if (mesh->mPrimitiveTypes <= aiPrimitiveType_LINE) continue;

        //TODO: For now. I think I saw someone checking normals and then setting this accordingly.
        glEnable(GL_LIGHTING);

        // where doing this man
        // where MAKING THIS HAPEN

        const struct aiMaterial *mtl = sc->mMaterials[mesh->mMaterialIndex];
        aiString texPath;

        //texIndex ?!?

        // Get a "handle" to the texture variables inside our shader.  Then
        // pass two textures to the shader: one for diffuse, and the other for
        // transparency.

        if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0 /*texIndex*/, &texPath)) {
            string strpath(texPath.data);
            GLint diffuse = glGetUniformLocation(shader->programID(), "diffuseMap");
            glUniform1i(diffuse, 0); // The diffuse map will be GL_TEXTURE0
            glActiveTexture(GL_TEXTURE0);
            textureIdMap[make_pair(strpath, aiTextureType_DIFFUSE)]->Bind();
        }

        if(AI_SUCCESS == mtl->GetTexture(aiTextureType_SPECULAR, 0 /*texIndex*/, &texPath)) {
            string strpath(texPath.data);
            GLint specular = glGetUniformLocation(shader->programID(), "specularMap");
            glUniform1i(specular, 0); // The diffuse map will be GL_TEXTURE0
            glActiveTexture(GL_TEXTURE1);
            textureIdMap[make_pair(strpath, aiTextureType_SPECULAR)]->Bind();
        }

        //setMaterial(sc, mesh);
        drawMesh(mesh);
    }

    for (int n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n]);
    }

    glPopMatrix();
}

//
//Code borrowed with some modification from http://assimp.svn.sourceforge.net/viewvc/assimp/trunk/samples/SimpleTexturedOpenGL/SimpleTexturedOpenGL/src/model_loading.cpp?revision=1171&content-type=text%2Fplain
//

void LoadGLTextures(const aiScene* scene) {
    assert(!scene->HasTextures());

    //Map each path to a loaded texture file.
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        for (unsigned int k = 0; k < scene->mMaterials[i]->mNumAllocated; k++) {
            cout << " i is " << i << endl;
            aiString path;

            if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, k, &path)) {
                string strpath(path.data);
                cout << "path is " << path.data << endl;
                string filename(BASE_PATH);
                filename += path.data;
                filename += "_d.jpg"; //TODO: Dehack.

                cout << filename << endl;

                sf::Image *img_diff = new sf::Image();
                img_diff->LoadFromFile(filename);
                textureIdMap[make_pair(strpath, aiTextureType_DIFFUSE)] = img_diff;
            }

            if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_SPECULAR, k, &path)) {
                string strpath(path.data);
                cout << "path is " << path.data << endl;
                string filename(BASE_PATH);
                filename += path.data;
                filename += "_s.jpg"; //TODO: Dehack.

                sf::Image *img_spec = new sf::Image();
                img_spec->LoadFromFile(path.data);
                textureIdMap[make_pair(strpath, aiTextureType_SPECULAR)] = img_spec;
            }
        }
    }
    /*for (unsigned int m=0; m<scene->mNumMaterials; m++) {
        int texIndex = 0;
        aiReturn texFound = AI_SUCCESS;

        aiString path;  // filename

        while (texFound == AI_SUCCESS)
        {
            texFound = scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, texIndex, &path);
            sf::Image *img = new sf::Image();
            img->LoadFromFile(path.data);
            textureIdMap[path.data] = img; //fill map with textures, pointers still NULL yet
            texIndex++;
        }
    }
    */
}

/*
void apply_material(const aiMaterial *mtl)
{
    float c[4];

    GLenum fill_mode;
    int ret1, ret2;
    aiColor4D diffuse;
    aiColor4D specular;
    aiColor4D ambient;
    aiColor4D emission;
    float shininess, strength;
    int two_sided;
    int wireframe;
    unsigned int max;   // changed: to unsigned

    int texIndex = 0;
    aiString texPath;   //contains filename of texture

    if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, texIndex, &texPath))
    {
        //bind texture
        unsigned int texId = *textureIdMap[texPath.data];
        glBindTexture(GL_TEXTURE_2D, texId);
    }

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
    max = 1;
    ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
    if((ret1 == AI_SUCCESS) && (ret2 == AI_SUCCESS))
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
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
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}
*/

void setMaterial(const struct aiScene *scene, const struct aiMesh *mesh) {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiColor3D color;

    // Get a handle to the diffuse, specular, and ambient variables
    // inside the shader.  Then set them with the diffuse, specular, and
    // ambient color.
    GLint diffuse = glGetUniformLocation(shader->programID(), "Kd");
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glUniform3f(diffuse, color.r, color.g, color.b);

    // Specular material
    GLint specular = glGetUniformLocation(shader->programID(), "Ks");
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glUniform3f(specular, color.r, color.g, color.b);

    // Ambient material
    GLint ambient = glGetUniformLocation(shader->programID(), "Ka");
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glUniform3f(ambient, color.r, color.g, color.b);

    // Specular power
    GLint shininess = glGetUniformLocation(shader->programID(), "alpha");
    float value;
    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, value)) {
        glUniform1f(shininess, value);
    } else {
        glUniform1f(shininess, 1);
    }
}

void setTextures() {
    // Get a "handle" to the texture variables inside our shader.  Then
    // pass two textures to the shader: one for diffuse, and the other for
    // transparency.
    GLint diffuse = glGetUniformLocation(shader->programID(), "diffuseMap");
    glUniform1i(diffuse, 0); // The diffuse map will be GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    diff_text->Bind();

    // Transparency
    GLint specular = glGetUniformLocation(shader->programID(), "specularMap");
    glUniform1i(specular, 1); // The transparency map will be GL_TEXTURE1
    glActiveTexture(GL_TEXTURE1);
    spec_text->Bind();
}

void drawMesh(const struct aiMesh *mesh) {
    vector<unsigned> indexBuffer;

    indexBuffer.reserve(mesh->mNumFaces * 3);
    for (unsigned i = 0; i < mesh->mNumFaces; i++) {
        for (unsigned j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
            indexBuffer.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }

    // Get a handle to the variables for the vertex data inside the shader.
    GLint position = glGetAttribLocation(shader->programID(), "positionIn");
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 3, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mVertices);

    // Texture coords.  Note the [0] at the end, very important
    GLint texcoord = glGetAttribLocation(shader->programID(), "texcoordIn");
    glEnableVertexAttribArray(texcoord);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mTextureCoords[0]);

    // Normals
    GLint normal = glGetAttribLocation(shader->programID(), "normalIn");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(normal, 3, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mNormals);

    glDrawElements(GL_TRIANGLES, 3 * mesh->mNumFaces, GL_UNSIGNED_INT, &indexBuffer[0]);
}


void renderFrame() {
    //////////////////////////////////////////////////////////////////////////
    // TODO: ADD YOUR RENDERING CODE HERE.  You may use as many .cpp files
    // in this assignment as you wish.
    //////////////////////////////////////////////////////////////////////////

    glUseProgram(shader->programID());

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

    if (drx != 0 || dry != 0) {
        rx += drx * 5;
        ry += dry * 5;
        drx = 0;
        dry = 0;
    }
    glRotatef(rx, 0, rx, 0);
    glRotatef(ry, 0, ry, 0);

    //setTextures();

    for (int i = 0; i < assets.size(); ++i) {
        recursive_render(assets[i].scene, assets[i].scene->mRootNode);
    }
}






