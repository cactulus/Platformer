bool checkOpenGLError();
std::string read_file(const char *file_path);
float to_radians(float degrees);

struct Platformer;
struct World;
void reset_world(World *world);
void load_world(Platformer *platformer, const char *file_name);