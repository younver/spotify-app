#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <vector>
#include <string>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <mysql.h>
#include <GLFW/glfw3.h> // System OpenGL headers
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void Colors(){
    
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowMinSize = ImVec2(900, 540);

    style.Colors[ImGuiCol_WindowBg] = ImColor(16, 16, 16);
    style.Colors[ImGuiCol_ChildBg] = ImColor(24, 24, 24);
    style.Colors[ImGuiCol_Text] = ImColor(255, 255, 255);
    style.Colors[ImGuiCol_CheckMark] = ImColor(255, 255, 255);

    style.Colors[ImGuiCol_Header] = ImColor(48, 48, 48);
    style.Colors[ImGuiCol_HeaderActive] = ImColor(32, 32, 32);
    style.Colors[ImGuiCol_HeaderHovered] = ImColor(32, 32, 32);

    style.Colors[ImGuiCol_FrameBg] = ImColor(240, 240, 240);
    style.FrameRounding = 1;

    style.Colors[ImGuiCol_Button] = ImColor(0.51f, 0.48f, 0.6f, 1.0f);
}

//----------------------------------------


//----------------------------------------

int main(int, char**)
{

    // Database initialization
    sql::Driver *driver;
    sql::Connection *connection;
    sql::Statement *statement;
    sql::PreparedStatement *preparedStatement;
    sql::ResultSet *result;

    // Database connection
    driver = get_driver_instance();
    connection = driver->connect();
    connection->setSchema("spotify");

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    
    //----------------------------------------

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    //----------------------------------------

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Spotify App", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Set style
    Colors();

    // Load Font
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/Ubuntu-Regular.ttf", 18.0f);
    IM_ASSERT(font != NULL);

    // Our state
    ImVec4 clear_color = ImVec4(0.51f, 0.48f, 0.6f, 1.0f);

    //----------------------------------------

    struct album {
        std::string id;
        std::string name;
        ImTextureID image_texture;
        std::string creator_name;
    };

    std::vector<album> albums = {};

    // Initialize top albums
    statement = connection->createStatement();
    result = statement->executeQuery("SELECT * FROM top_albums");
    while (result->next()) {
        
        std::string album_id = result->getString("album_id");
        std::string album_name = result->getString("album_name");
        
        std::cout << album_name << " " << album_id << std::endl;

        album new_album = {album_id, album_name};

        albums.push_back(new_album);
    }

    // Initialize album window

    // Load album image
    // albums[0].image_texture = get_album_image(albums[0].id);
    
    // Load creator name
    statement = connection->createStatement();
    statement->execute("CALL load_creator_name('" + albums[0].id + "', @creator_name)");
    result = statement->executeQuery("SELECT @creator_name");
    result->next();
    albums[0].creator_name = result->getString("@creator_name");

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        {
            static int current_album_index = 0;

            ImGui::Begin("Spotify App", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize );
            {

                ImGui::PushStyleColor(ImGuiCol_Border, ImColor(0, 0, 0, 255).Value );

                ImGui::BeginChild("##albumList", ImVec2(260, ImGui::GetContentRegionAvail().y), true);
                {
                    for (unsigned int i = 0; i < albums.size(); i++){

                        bool selected = (current_album_index == i);
                        
                        if (ImGui::Selectable(albums[i].name.c_str(), &selected)){
                            current_album_index = i;

                            // Load album image
                            // albums[i].image_texture = get_album_image(albums[0].id);
                            
                            // Load creator name
                            statement = connection->createStatement();
                            statement->execute("CALL load_creator_name('" + albums[i].id + "', @creator_name)");
                            result = statement->executeQuery("SELECT @creator_name");
                            result->next();
                            albums[i].creator_name = result->getString("@creator_name");
                        }

                        if (selected){
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                }
                ImGui::EndChild();

                ImGui::SameLine();

                ImGui::BeginChild("##albumView", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), true);
                {
                    auto end = [] (float available_width, float element_width, float padding = 0, float spacing = 15) {
                        ImGui::SameLine( (available_width) - (element_width / 2) + padding );
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
                    };

                    auto centre = [] (float available_width, float element_width, float padding = 0) {
                        ImGui::SameLine( (available_width / 2) - (element_width / 2) + padding );
                    };

                    auto centre_text = [ & ] (const char* format, float spacing = 15, ImColor colour = ImColor(255, 255, 255)) {
                        centre(ImGui::GetContentRegionAvail().x, ImGui::CalcTextSize(format).x);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
                        ImGui::TextColored(colour.Value, format);
                    };
                    
                    // Search bar
                    static char input_text[256] = "";
                    end(ImGui::GetContentRegionAvail().x, 300);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImColor(0,0,0, 255).Value);
                    ImGui::InputText("##searchBar", input_text, 256);
                    ImGui::PopStyleColor();

                    // Search button
                    end(ImGui::GetContentRegionAvail().x, 300, 30, 45);
                    if (ImGui::Button("Return", ImVec2(100, 30))) {
                        
                        // Find the album name
                        preparedStatement = connection->prepareStatement("SET @incomplete_album_name = ?");
                        preparedStatement->setString(1, input_text);
                        preparedStatement->execute();

                        statement = connection->createStatement();
                        statement->execute("CALL complete_album_name(@incomplete_album_name)");
                        
                        result = statement->executeQuery("SELECT @incomplete_album_name");
                        result->next();
                        std::string found_album_name = result->getString("@incomplete_album_name");

                        // Find the album id
                        statement = connection->createStatement();
                        statement->execute("SET @found_album_id = ''");
                        preparedStatement = connection->prepareStatement("CALL get_album_id(?, @found_album_id)");
                        preparedStatement->setString(1, found_album_name);
                        preparedStatement->execute();
                        
                        result = statement->executeQuery("SELECT @found_album_id");
                        result->next();
                        std::string found_album_id = result->getString("@found_album_id");

                        // Load creator name
                        statement = connection->createStatement();
                        statement->execute("CALL load_creator_name('" + found_album_id + "', @creator_name)");
                        result = statement->executeQuery("SELECT @creator_name");
                        result->next();
                        std::string found_creator_name = result->getString("@creator_name");

                        album found_album;
                        found_album.id = found_album_id;
                        found_album.name = found_album_name;
                        found_album.creator_name = found_creator_name;

                        albums.push_back(found_album);
                        current_album_index = albums.size();
                    }

                    auto& current_album = albums[current_album_index];

                    // Image
                    centre(ImGui::GetContentRegionAvail().x, 300);
                    ImGui::Image(current_album.image_texture, ImVec2(300,300) );

                    // Texts
                    centre_text( current_album.name.c_str(), 300 );
                    centre_text( current_album.id.c_str(), 340 );
                    centre_text( current_album.creator_name.c_str(), 380 );
                    
                }
                ImGui::EndChild();
                
                ImGui::PopStyleColor();
            
            }
            ImGui::End();
        }
        


        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    delete result;
    delete statement;
    delete preparedStatement;
    delete connection;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
