#include "draw/map.h"

#include "game_client.h"
#include "draw/sprite.h"
#include "scene/game.h"

namespace ace { namespace draw {
    using namespace ace::draw::detail;

    namespace {
        void gen_faces(const float x, const float y, const float z, const uint8_t vis, const glm::vec3 color, std::vector<VXLVertex> &v) {
            const float x0 = x, x1 = x + 1.0f;
            const float y0 = -z - 1.0f, y1 = -z;
            const float z0 = y, z1 = y + 1.0f;

            // vis = 0b11111111;

            if (vis & 1 << int(Face::LEFT)) {
                v.push_back({ { x0, y0, z0 }, color, 0 });
                v.push_back({ { x0, y1, z0 }, color, 0 });
                v.push_back({ { x0, y0, z1 }, color, 0 });
                v.push_back({ { x0, y0, z1 }, color, 0 });
                v.push_back({ { x0, y1, z0 }, color, 0 });
                v.push_back({ { x0, y1, z1 }, color, 0 });
            }
            if (vis & 1 << int(Face::RIGHT)) {
                v.push_back({ { x1, y0, z0 }, color, 1 });
                v.push_back({ { x1, y0, z1 }, color, 1 });
                v.push_back({ { x1, y1, z0 }, color, 1 });
                v.push_back({ { x1, y1, z0 }, color, 1 });
                v.push_back({ { x1, y0, z1 }, color, 1 });
                v.push_back({ { x1, y1, z1 }, color, 1 });
            }
            if (vis & 1 << int(Face::BACK)) {
                v.push_back({ { x0, y0, z0 }, color, 2 });
                v.push_back({ { x1, y0, z0 }, color, 2 });
                v.push_back({ { x0, y1, z0 }, color, 2 });
                v.push_back({ { x0, y1, z0 }, color, 2 });
                v.push_back({ { x1, y0, z0 }, color, 2 });
                v.push_back({ { x1, y1, z0 }, color, 2 });
            }
            if (vis & 1 << int(Face::FRONT)) {
                v.push_back({ { x0, y0, z1 }, color, 3 });
                v.push_back({ { x0, y1, z1 }, color, 3 });
                v.push_back({ { x1, y0, z1 }, color, 3 });
                v.push_back({ { x1, y0, z1 }, color, 3 });
                v.push_back({ { x0, y1, z1 }, color, 3 });
                v.push_back({ { x1, y1, z1 }, color, 3 });
            }
            if (vis & 1 << int(Face::TOP)) {
                v.push_back({ { x0, y1, z0 }, color, 4 });
                v.push_back({ { x1, y1, z0 }, color, 4 });
                v.push_back({ { x0, y1, z1 }, color, 4 });
                v.push_back({ { x0, y1, z1 }, color, 4 });
                v.push_back({ { x1, y1, z0 }, color, 4 });
                v.push_back({ { x1, y1, z1 }, color, 4 });
            }
            if (vis & 1 << int(Face::BOTTOM)) {
                v.push_back({ { x0, y0, z0 }, color, 5 });
                v.push_back({ { x0, y0, z1 }, color, 5 });
                v.push_back({ { x1, y0, z0 }, color, 5 });
                v.push_back({ { x1, y0, z0 }, color, 5 });
                v.push_back({ { x0, y0, z1 }, color, 5 });
                v.push_back({ { x1, y0, z1 }, color, 5 });
            }
        }
    }

    VXLBlocks::VXLBlocks(const std::vector<VXLBlock> &blocks, const glm::vec3 &center) : scale(1), rotation(0), position(0) {
        this->vao.attrib_pointer("3f,3f,1B", this->vbo.handle);
        this->update(blocks, center);
    }

    void VXLBlocks::update(const std::vector<VXLBlock> &blocks, const glm::vec3 &center, bool gen_vis) {
        this->centroid = center;

        std::unordered_set<glm::ivec3> bmap;
        if(gen_vis) {
            for (const VXLBlock &block : blocks) {
                bmap.emplace(block.position.x, block.position.y, block.position.z);
            }
        }

        for (const VXLBlock &block : blocks) {
            uint8_t r, g, b, a;
            unpack_bytes(block.color, &a, &r, &g, &b);

            gen_faces(
                block.position.x - this->centroid.x,
                block.position.y - this->centroid.y,
                block.position.z - this->centroid.z,
                gen_vis ? VXLBlocks::get_vis(bmap, block.position) : block.vis, glm::vec3{ r, g, b } / 255.f, this->vbo.data
            );
        }
        this->vbo.upload();
    }

    void VXLBlocks::draw(gl::ShaderProgram& s) const {
        s.uniform("model", model_matrix(this->position, this->rotation, this->scale));
        this->vao.draw(GL_TRIANGLES, this->vbo.draw_count);
    }

    uint8_t VXLBlocks::get_vis(std::unordered_set<glm::ivec3> &set, glm::ivec3 pos) {
        if (!set.count(pos)) return 0;

        uint8_t vis = 0;
        if (!set.count({pos.x - 1, pos.y, pos.z})) vis |= 1 << int(Face::LEFT);
        if (!set.count({pos.x + 1, pos.y, pos.z})) vis |= 1 << int(Face::RIGHT);
        if (!set.count({pos.x, pos.y - 1, pos.z})) vis |= 1 << int(Face::BACK);
        if (!set.count({pos.x, pos.y + 1, pos.z})) vis |= 1 << int(Face::FRONT);
        if (!set.count({pos.x, pos.y, pos.z - 1})) vis |= 1 << int(Face::TOP);
        if (!set.count({pos.x, pos.y, pos.z + 1})) vis |= 1 << int(Face::BOTTOM);
        return vis;
    }

    Pillar::Pillar(AceMap &map, size_t x, size_t y) : dirty(true), map(map), x(x), y(y) {
        this->vao.attrib_pointer("3f,3f,1B", this->vbo.handle);
    }

    void Pillar::update() {
        for (size_t ax = this->x; ax < this->x + PILLAR_SIZE; ax++) {
            for (size_t ay = this->y; ay < this->y + PILLAR_SIZE; ay++) {
                for (size_t az = 0; az < MAP_Z; az++) {
                    uint8_t vis = map.get_vis(ax, ay, az, true);
                    if (az == MAP_Z - 1) vis &= 1 << int(Face::TOP);
                    if (vis == 0) continue;

                    const uint32_t col = map.get_color(ax, ay, az);
                    uint8_t r, g, b, a;
                    unpack_bytes(col, &a, &r, &g, &b);

                    gen_faces(ax, ay, az, vis, (glm::vec3{ r, g, b } * (map.sunblock(ax, ay, az) / 127.f) * (a / 127.f)) / 255.f, this->vbo.data);
                }
            }
        }

        this->vbo.upload();
        this->dirty = false;
    }

    void Pillar::draw() {
        if (dirty) this->update();

        this->vao.draw(GL_TRIANGLES, this->vbo.draw_count);
    }

    std::unique_ptr<uint8_t[]> read_file(const std::string &file_path) {
        FILE *f = fopen(file_path.c_str(), "rb");
        if (!f) THROW_ERROR("COULD NOT READ MAP FILE {}\n", file_path);

        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);
        auto buf = std::make_unique<uint8_t[]>(len);
        fread(buf.get(), len, 1, f);
        fclose(f);
        return buf;
    }

    DrawMap::DrawMap(scene::GameScene &s, const std::string &file_path) : DrawMap(s, read_file(file_path).get()) {
    }

    DrawMap::DrawMap(scene::GameScene &s, uint8_t *buf) : AceMap(buf), scene(s) {
        this->gen_pillars();
    }

    void DrawMap::update(double dt) {
        for (auto i = damage_queue.begin(); i != damage_queue.end();) {
            if (scene.time >= i->first) {
                int x = i->second.x, y = i->second.y, z = i->second.z;
                if(this->get_solid(x, y, z)) {
                    this->set_point(x, y, z, true, (0x7F << 24) | this->get_color(x, y, z));
                }
                i = damage_queue.erase(i);
            } else {
                ++i;
            }
        }
    }

    void DrawMap::draw(gl::ShaderProgram &shader) {
        // for (auto &p : this->pillars) {
        //     if (p.contains(draw2vox(this->scene.cam.position))) {
        //         this->scene.debug.draw_quad({ p.x, 0, p.y }, { p.x, -64, p.y }, { p.x + PILLAR_SIZE, -64, p.y }, { p.x + PILLAR_SIZE, 0, p.y }, glm::vec3{ 0, 1, 0 });
        //     }
        //
        // }

        for (auto &p : this->pillars) {
            if (p.contains(draw2vox(this->scene.cam.position))) {
                this->scene.debug.draw_cube({ p.x + 8, -32, p.y + 8 }, { PILLAR_SIZE, 64, PILLAR_SIZE }, { 1, 0, 0 });
            }
            if (this->scene.cam.box_in_frustum(p.x, 0, p.y, p.x + PILLAR_SIZE, -64, p.y + PILLAR_SIZE)) {
                p.draw();
            }
        }


    }

    Pillar &DrawMap::get_pillar(const int x, const int y, const int z) {
        int xp = (x & MAP_X - 1) / PILLAR_SIZE;
        int yp = (y & MAP_Y - 1) / PILLAR_SIZE;
        return this->pillars[xp * (MAP_Y / PILLAR_SIZE) + yp];
    }

    void DrawMap::gen_pillars() {
        pillars.clear();
        pillars.reserve((MAP_X / PILLAR_SIZE) * (MAP_Y / PILLAR_SIZE));
        for (size_t x = 0; x < MAP_X / PILLAR_SIZE; x++) {
            for (size_t y = 0; y < MAP_Y / PILLAR_SIZE; y++) {
                pillars.emplace_back(*this, x * PILLAR_SIZE, y * PILLAR_SIZE);
            }
        }
    }


    bool DrawMap::set_point(const int x, const int y, const int z, const bool solid, const uint32_t color) {
        bool ok = AceMap::set_point(x, y, z, solid, color);

        this->get_pillar(x - 1, y, z).dirty |= ok;
        this->get_pillar(x + 1, y, z).dirty |= ok;
        this->get_pillar(x, y - 1, z).dirty |= ok;
        this->get_pillar(x, y + 1, z).dirty |= ok;
        // pillars dont render edges even across pillar boundries
        // so you have to update adjacent pillars if the destroyed block shares a face with another pillar.
        // TODO: shadows dont update across chunks. the origin block can be ~18 blocks a way, so maybe update all chunks within
        // 18 blocks? or only in the direction of shadows (increasing y)

        if (x == 0 || y == 0 || x == MAP_X - 1 || y == MAP_Y - 1 || !(x & 63) || !(y & 63)) {
            return ok;
        }

        glm::u8vec4 pixel = unpack_argb(this->get_color(x, y, this->get_z(x, y)));
        pixel.a = 255;
        this->scene.hud.map_display.map->tex.set_pixel(x, y, pixel);

        return ok;
    }

    bool DrawMap::build_point(const int x, const int y, const int z, glm::u8vec3 color, bool force) {
        if (!force) {
            if (!valid_build_pos(x, y, z)) return false;

            std::vector<glm::ivec3> neighbors;
            this->add_neighbors(neighbors, x, y, z);
            if (neighbors.empty()) return false;
        }

        return this->set_point(x, y, z, true, pack_bytes(0x7F, color.r, color.g, color.b));
    }

    bool DrawMap::destroy_point(const int x, const int y, const int z, std::vector<VXLBlock> &destroyed) {
        if (!valid_build_pos(x, y, z)) return false;

        bool ok = this->set_point(x, y, z, false, 0);

        std::vector<glm::ivec3> neighbors;
        this->add_neighbors(neighbors, x, y, z);
        for (const auto &node : neighbors) {
            if (valid_build_pos(node.x, node.y, node.z)) {
                this->check_node(node.x, node.y, node.z, true, destroyed);
            }
        }

        return ok;
    }

    bool DrawMap::damage_point(int x, int y, int z, uint8_t damage) {
        if (!valid_build_pos(x, y, z) || !this->get_solid(x, y, z)) return false;

        uint32_t color = this->get_color(x, y, z);
        int health = color >> 24;
        health -= damage;
        if(health <= 0) {
            return true;
        }
        color = ((health & 0xFF) << 24) | (color & 0x00FFFFFF);
        this->set_point(x, y, z, true, color);
        damage_queue.push_back({ scene.time + 10, {x, y, z} });
        return false;
    }
    
    draw::SpriteGroup *DrawMap::get_overview() {
        auto pixels(std::make_unique<uint8_t[]>(MAP_X * MAP_Y * 3));
        int p = 0;
        for(int y = 0; y < MAP_Y; y++) {
            for(int x = 0; x < MAP_X; x++) {
                uint8_t r, g, b;
                if(x == 0 || y == 0 || x == MAP_X - 1 || y == MAP_Y - 1) {
                    r = g = b = 0;
                } else if((x & 63) == 0 || (y & 63) == 0) {
                    r = g = b = 255;
                } else {
                    uint8_t a;
                    unpack_bytes(this->get_color(x, y, this->get_z(x, y)), &a, &r, &g, &b);
                }
                
                pixels[p++] = r; pixels[p++] = g; pixels[p++] = b;
            }
        }
        auto overview = this->scene.client.sprites.get("map_overview", SDL_CreateRGBSurfaceFrom(pixels.get(), MAP_X, MAP_Y, 24, 3 * MAP_X, 0xFF, 0xFF << 8, 0xFF << 16, 0));
        overview->set_antialias(false);
        return overview;
    }
}}
