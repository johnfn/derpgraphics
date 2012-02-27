#include "Framework.h"
#include "Shader.h"
#include <assert.h>
#include "assimp.h"
#include "aiPostProcess.h"
#include "aiScene.h"
#include <vector>
#include <fstream>

//TODO
#define DEBUG

//TODO
#define BASE_PATH "/Users/grantm/class/cs248/assign3/models/"
#define MODEL_PATH_CATH "/Users/grantm/class/cs248/assign3/models/cathedral.3ds"
#define MODEL_PATH_SPHE "/Users/grantm/class/cs248/assign3/models/sphere.3ds"
#define MODEL_PATH_ARMA "/Users/grantm/class/cs248/assign3/models/armadillo.3ds"
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

Assimp::Importer importers[2];

// This creates an asset importer using the Open Asset Import library.
// It automatically manages resources for you, and frees them when the program
// exits.

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

GLenum to_texture_num(string str, aiTextureType type, bool die_if_doesnt_exist = false) {
    pair<string, aiTextureType> p = make_pair(str, type);

    static map<pair<string, aiTextureType>, GLenum > m;
    static int highest_so_far = GL_TEXTURE0;

    if (!m.count(p)) {
        if (die_if_doesnt_exist) {
            assert("Couldn't find texture!!!");
        }

        cout << highest_so_far + 1 << endl;

        m[p] = highest_so_far;
        highest_so_far++;
    }

    return m[p];
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

asset loadAsset(const char* name, int imp) {
    asset a;

    a.scene = importers[imp].ReadFile(name,
        aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcessPreset_TargetRealtime_Quality);

    if (!a.scene || a.scene->mNumMeshes <= 0) {
        std::cerr << importers[imp].GetErrorString() << std::endl;
        exit(-1);
    }

    cout << "GO" << endl;
    LoadGLTextures(a.scene);

    return a;
}

void loadAssets() {
    //asset cathedral = loadAsset(MODEL_PATH_CATH);
    //assets.push_back(cathedral);

    asset a = loadAsset(MODEL_PATH_SPHE, 0);
    assets.push_back(a);

    asset c = loadAsset(MODEL_PATH_CATH, 1);
    assets.push_back(c);

    //asset sphere = loadAsset(MODEL_PATH_2);
    //assets.push_back(sphere);

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

float mx = 0.0f;
float my = 0.0f;

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
            case sf::Event::MouseMoved:
                mx = evt.MouseMove.X * 360.0f / 800.0f;
                my = evt.MouseMove.Y * 360.0f / 800.0f;
                break;
            /*case sf::Event::KeyPressed:
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
                break;*/
            default:
                break;
        }
    }
}

//strType == diffuseMap (for instance.)
void apply_texture(const aiTextureType type, const string strType, const string strpath) {
    GLenum textureNum = to_texture_num(strpath, type);

    GLint loc = glGetUniformLocation(shader->programID(), strType.c_str());
    glUniform1i(loc, textureNum - GL_TEXTURE0); // The diffuse map will be

    glActiveTexture(textureNum);

    sf::Image *img = textureIdMap[make_pair(strpath, type)];

    /* With thanks to http://stackoverflow.com/questions/5436487/how-would-i-be-able-to-use-glu-rgba-or-other-glu-parameters */
    //TODO: Far too slow. Preprocess.
    //gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, img->GetWidth(), img->GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, img->GetPixelsPtr());
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    //TODO???????

    /*
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    */

    img->Bind();
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

        // where doing this man
        // where MAKING THIS HAPEN

        const struct aiMaterial *mtl = sc->mMaterials[mesh->mMaterialIndex];
        aiString texPath;

        //texIndex ?!?

        // Get a "handle" to the texture variables inside our shader.  Then
        // pass two textures to the shader: one for diffuse, and the other for
        // transparency.

        //apply_material(mtl);
        setMaterial(sc, mesh);

        if(AI_SUCCESS == mtl->GetTexture(aiTextureType_DIFFUSE, 0 /*texIndex*/, &texPath)) {
            /* Diffuse */

            string strpath(texPath.data);

            apply_texture(aiTextureType_DIFFUSE, "diffuseMap", strpath);

            /* Specular */
            apply_texture(aiTextureType_SPECULAR, "specularMap", strpath);

            if (textureIdMap.count(make_pair(strpath, aiTextureType_NORMALS)) != 0) {
                GLint hasNormal = glGetUniformLocation(shader->programID(), "hasNormalMapping");
                glUniform1i(hasNormal, true);

                apply_texture(aiTextureType_NORMALS, "normalMap", strpath);
            }
        }

        drawMesh(mesh);
    }

    for (int n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n]);
    }

    glPopMatrix();
}


string stringify(const char* s) {
    string str(s);
    return str;
}

bool fexists(const char *filename) {
  ifstream ifile(filename);
  return ifile;
}

//
//Code borrowed with some modification from http://assimp.svn.sourceforge.net/viewvc/assimp/trunk/samples/SimpleTexturedOpenGL/SimpleTexturedOpenGL/src/model_loading.cpp?revision=1171&content-type=text%2Fplain
//
void loadAndStoreImage(string filename, string basepath, aiTextureType type, bool filter = true) {
    sf::Image *img = new sf::Image();
    img->LoadFromFile(filename);
    textureIdMap[make_pair(basepath, type)] = img;

    to_texture_num(basepath, type);

    //glActiveTexture(texnum); // It seems I need to set *some* active texture, but it doesn't matter which one. (TODO?)
    //img->Bind();

    /* With thanks to http://stackoverflow.com/questions/5436487/how-would-i-be-able-to-use-glu-rgba-or-other-glu-parameters */
    if (filter) {
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, img->GetWidth(), img->GetHeight(), GL_RGBA, GL_UNSIGNED_BYTE, img->GetPixelsPtr());
    }

}

void LoadGLTextures(const aiScene* scene) {
    assert(!scene->HasTextures());

    //Map each path to a loaded texture file.
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        for (unsigned int k = 0; k < scene->mMaterials[i]->mNumAllocated; k++) {
            aiString path;

            if (AI_SUCCESS == scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, k, &path)) {
                string basepath(path.data);
                string filename(BASE_PATH);

                /* Diffuse texture */
                filename = stringify(BASE_PATH) + stringify(path.data) + stringify("_d.jpg");

                loadAndStoreImage(filename, basepath, aiTextureType_DIFFUSE);

                /* Specular texture */
                filename = stringify(BASE_PATH) + stringify(path.data) + stringify("_s.jpg");

                loadAndStoreImage(filename, basepath, aiTextureType_SPECULAR);

                filename = stringify(BASE_PATH) + stringify(path.data) + stringify("_n.jpg");

                /* Normal texture? */
                if (fexists(filename.c_str())) {
                    loadAndStoreImage(filename, basepath, aiTextureType_NORMALS, false);
                }
            }
        }
    }
}

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

    //Tangent
    GLint tangent = glGetAttribLocation(shader->programID(), "tangentIn");
    glEnableVertexAttribArray(tangent);
    glVertexAttribPointer(tangent, 3, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mTangents);

    //Bitangent
    GLint biTangent = glGetAttribLocation(shader->programID(), "biTangentIn");
    glEnableVertexAttribArray(biTangent);
    glVertexAttribPointer(biTangent, 3, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mBitangents);

    glDrawElements(GL_TRIANGLES, 3 * mesh->mNumFaces, GL_UNSIGNED_INT, &indexBuffer[0]);
}


void renderFrame() {
    //////////////////////////////////////////////////////////////////////////
    // TODO: ADD YOUR RENDERING CODE HERE.  You may use as many .cpp files
    // in this assignment as you wish.
    //////////////////////////////////////////////////////////////////////////

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader->programID());

    GLfloat aspectRatio = (GLfloat)window.GetWidth()/window.GetHeight();
    GLfloat nearClip = 0.1f;
    GLfloat farClip = 500.0f;
    GLfloat fieldOfView = 45.0f; // Degrees

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fieldOfView, aspectRatio, nearClip, farClip);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0f, 2.0f, -12.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // Add a little rotation, using the elapsed time for smooth animation

    glRotatef(mx, 0, 1, 0);
    glRotatef(my, 0, 0, 1);

    glEnable(GL_LIGHTING);
    /*
    GLfloat light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
    GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    GLfloat pos[4] = {0.0f, 11.0f, 0.0f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    */

/*

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
    */

    //setTextures();

    for (int i = 0; i < assets.size(); ++i) {
        recursive_render(assets[i].scene, assets[i].scene->mRootNode);
    }
}






