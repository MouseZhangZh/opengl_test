#include "glad/glad.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "shader.h"

#include <iostream>
#include <array>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path, bool gammaCorrection);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 640;

const std::string SHADER_PATH{"/Users/zhangzihao/Code/Projects/opengl_test/opengl_test/src/shaders/"};
const std::string IMAGE_PATH{"/Users/zhangzihao/Code/Projects/opengl_test/opengl_test/src/images/"};

// stores how much we're seeing of either texture
float mixValue = 0.2f;

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    // you can name your shader files however you like
    Shader ourShader((SHADER_PATH + "shader.vert").c_str(), (SHADER_PATH + "shader.frag").c_str());
    Shader blurShader((SHADER_PATH + "blur_shader.vert").c_str(), (SHADER_PATH + "blur_shader.frag").c_str());

//    static constexpr std::uint8_t scaleFactor = 16;
    unsigned int scaleFactor = 8;
    unsigned int copyVAO = 0, copyVBO = 0;
    unsigned int highResFBO{};
    unsigned int highResBuf{};
    unsigned int pingpongFBO[2]{};
    unsigned int pingpongBuffer[2]{};
//    static constexpr std::uint8_t blurIterations = 16;
    std::uint8_t blurIterations = 16;
    static constexpr std::array<float, 20> quadVertices {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f
    };

    // PING PONG
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongBuffer);
    for (std::uint8_t i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongBuffer[i]);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH / scaleFactor, SCR_HEIGHT / scaleFactor, 0, GL_RGB,
                GL_FLOAT, NULL);
//        glTexImage2D(
//                GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB,
//                GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                pingpongBuffer[i], 0);
    }

    // HIGH RESOLUTION
    glGenFramebuffers(1, &highResFBO);
    glGenTextures(1, &highResBuf);
    glBindFramebuffer(GL_FRAMEBUFFER, highResFBO);

    glBindTexture(GL_TEXTURE_2D, highResBuf);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, highResBuf, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    blurShader.use();
    blurShader.setInt("image", 0);

    // create copy vao
    glGenVertexArrays(1, &copyVAO);
    glGenBuffers(1, &copyVBO);
    glBindVertexArray(copyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, copyVBO);
    glBufferData(GL_ARRAY_BUFFER, quadVertices.size() * sizeof(float), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    // load and create a texture
    // -------------------------
    unsigned int texture1, texture2;
    stbi_set_flip_vertically_on_load(true);
    texture1 = loadTexture((IMAGE_PATH + "cloud.jpg").c_str(), false);
    texture2 = loadTexture((IMAGE_PATH + "street.jpg").c_str(), false);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use(); // don't forget to activate/use the shader before setting uniforms!
    // either set it manually like so:
    glUniform1i(glGetUniformLocation(ourShader.ID, "texture1"), 0);
    // or set it via the texture class
    ourShader.setInt("texture2", 1);

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    unsigned long frame = 0ul;
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        blurIterations = 2 * static_cast<std::uint8_t>(64 * abs(cos(currentFrame)) + 2);
        std::cout << "\rFPS is: " << 1 / deltaTime << "Frame is: " << ++frame << "Avg FPS is: " << frame / currentFrame;
        // input
        // -----
        processInput(window);

        // render
        // ------
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);

        ourShader.setFloat("mixValue", mixValue);

        // render container
        ourShader.use();
        glBindVertexArray(copyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // Copy default FBO to HighResFBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, highResFBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH,
                          SCR_HEIGHT,
                          GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);

        // Copy HighResFBO to PingPong0FBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, highResFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pingpongFBO[0]);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glViewport(0, 0, SCR_WIDTH / scaleFactor, SCR_HEIGHT / scaleFactor);

        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH / scaleFactor,
                          SCR_HEIGHT / scaleFactor,
                          GL_COLOR_BUFFER_BIT,
                          GL_LINEAR);
//        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH,
//                          SCR_HEIGHT,
//                          GL_COLOR_BUFFER_BIT,
//                          GL_LINEAR);

        // Blur texture
        bool horizontal = false;
        blurShader.use();
        for (std::uint8_t i = 0; i < blurIterations; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[!horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pingpongBuffer[horizontal]);
            glBindVertexArray(copyVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            horizontal = !horizontal;
        }

        // Copy PingPong1FBO to HighResFBO
        glBindFramebuffer(GL_READ_FRAMEBUFFER, pingpongFBO[1]);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, highResFBO);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

        glBlitFramebuffer(0, 0, SCR_WIDTH / scaleFactor, SCR_HEIGHT / scaleFactor, 0, 0, SCR_WIDTH,
                          SCR_HEIGHT,
                          GL_COLOR_BUFFER_BIT,
                          GL_LINEAR);
//        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH,
//                          SCR_HEIGHT,
//                          GL_COLOR_BUFFER_BIT,
//                          GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glBindVertexArray(copyVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
//    glDeleteVertexArrays(1, &VAO);
//    glDeleteBuffers(1, &VBO);
//    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
    {
        mixValue += 0.001f; // change this value accordingly (might be too slow or too fast based on system hardware)
        if(mixValue >= 1.0f)
            mixValue = 1.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
    {
        mixValue -= 0.001f; // change this value accordingly (might be too slow or too fast based on system hardware)
        if (mixValue <= 0.0f)
            mixValue = 0.0f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path, bool gammaCorrection)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum internalFormat;
        GLenum dataFormat;
        if (nrComponents == 1)
        {
            internalFormat = dataFormat = GL_RED;
        }
        else if (nrComponents == 3)
        {
            internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
            dataFormat = GL_RGB;
        }
        else if (nrComponents == 4)
        {
            internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
            dataFormat = GL_RGBA;
        }

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}
