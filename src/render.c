#include "olc/olc.h"
#include "sprite.h"
#include "world_object.h"
#include "render.h"
#include "config.h"
#include "logging.h"
#include "weapon.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

// ensure angle in interval [0; 2*M_PI)
double normilize_angle(double angle) {
    double pi2 = 2 * M_PI;
    while (angle < 0) { angle += pi2; }
    while (angle >= pi2) { angle -= pi2; }
    return angle;
}
screen_obj_t get_object_on_screen(player_t* player, point_t obj_pos, double obj_radis, double obj_height, enum PLACE_ON_SCREEN place) {
    double angle_from_player_to_obj = normilize_angle(get_angle_from_pos1_to_pos2(player->pos, obj_pos));
    double player_angle = normilize_angle(player->angle);
    double player_aov = player->angle_of_vision;

    double distance = get_distance_from_pos1_to_pos2(player->pos, obj_pos);
    obj_height = (obj_height / distance);

    double obj_width_angle = atan2(obj_radis, distance);
    double angle_to_obj_left = angle_from_player_to_obj - obj_width_angle;
    double angle_to_obj_right = angle_from_player_to_obj + obj_width_angle;

    double start_view_angle = normilize_angle(player_angle - player_aov / 2);
    double left_angle = normilize_angle(angle_to_obj_left - start_view_angle);
    double right_angle = normilize_angle(angle_to_obj_right - start_view_angle);

    if ((left_angle < 0 && right_angle < 0) || left_angle > player_aov && right_angle > player_aov) {
        screen_obj_t res;
        res.on_screen = 0;
        return res;
    }

    int lrow_left = (int)(olc_screen_width() * left_angle / player_aov + 0.5);
    int lrow_right = (int)(olc_screen_width() * right_angle / player_aov + 0.5);

    int lline_start = 0;
    int lline_end = 0;

    if (place == AIR) {
        lline_start = (int)(olc_screen_height() / 2 - obj_height + 0.5);
        lline_end = (int)(olc_screen_height() / 2 + obj_height + 0.5);
    }
    else {
        double dist_rate = 1 / distance;
        if (dist_rate > 1) {
            dist_rate = 1;
        }
        double dist_on_screen = (olc_screen_height() / 2) * dist_rate;
        if (dist_on_screen > olc_screen_height()) {
            dist_on_screen = olc_screen_height();
        }
        int center = olc_screen_height() / 2 + (int)dist_on_screen;
        lline_start = center - (int)obj_height;
        lline_end = center;
    }

    screen_obj_t res;
    res.on_screen = 1;
    res.row_left = lrow_left;
    res.row_right = lrow_right;
    res.line_start = lline_start;
    res.line_end = lline_end;
    res.distance = distance;
    return res;
}


void draw_object(player_t* player, point_t obj_pos, double obj_radis, char ch, enum COLOR col, int obj_height, enum PLACE_ON_SCREEN place) {
    screen_obj_t obj = get_object_on_screen(player, obj_pos, obj_radis, obj_height, place);
    if (!obj.on_screen) {
        return;
    }
    for (int i = obj.row_left; i <= obj.row_right; i++) {
        if (i < 0 || i >= olc_screen_width()) {
            continue;
        }
        for (int j = obj.line_start; j < obj.line_end; j++) {
            if (j < 0 || j >= olc_screen_height()) {
                continue;
            }
            if (obj.distance < get_world()->z_buffer[i][j]) {
                olc_draw(i, j, ch, col);
                get_world()->z_buffer[i][j] = obj.distance;
            }
        }
    }
}

void draw_enemies(world_t* world) {
    player_t* player = &world->player;
    for (int i = 0; i < world->enemy_array.len; i++) {
        enemy_t* enemy = &world->enemy_array.array[i];
        sprite_t* mob = world->sprites.mob1_back;
        double d_angle = normilize_angle(enemy->angle - player->angle);
        if (fabs(d_angle) > M_PI) {
            d_angle = 2 * M_PI - d_angle;
            d_angle *= (-1);
        }
        if (d_angle <= M_PI_4 + M_PI_2 && d_angle >= M_PI_4){
            mob = world->sprites.mob1_side2;
        }
        else if (d_angle >= -M_PI_4 - M_PI_2 && d_angle <= -M_PI_4) {
            mob = world->sprites.mob1_side1;
        }
        else if ((d_angle >= M_PI_4 + M_PI_2 && d_angle <= M_PI) || (d_angle <= -M_PI_4 - M_PI_2 && d_angle >= -M_PI)) {
            mob = world->sprites.mob1;
        }
        draw_sprite(mob, 0, enemy->pos, enemy->radius, 40, AIR);
    }
}

void draw_drop(world_t* world) {
    player_t* player = &world->player;
    for (int i = 0; i < world->drop_array.len; i++) {
        drop_t* drop = &world->drop_array.array[i];
        sprite_t* sprite;
        if (drop->type == 0) {
            sprite = world->sprites.drop1;
        } else {
            sprite = world->sprites.drop2;
        }
        draw_sprite(sprite, 0, drop->pos, drop->radius, 60, FLOOR);
    }
}

void draw_bullets(world_t* world) {
    player_t* player = &world->player;
    for (int i = 0; i < world->bullet_array.len; i++) {
        bullet_t* bullet = &world->bullet_array.array[i];
        sprite_t* bullet_sprite = world->sprites.bullet_pistol;
        if (bullet->type == PLAYER_RIFLE) {
            bullet_sprite = world->sprites.bullet_rifle;
        }else if (bullet->type == CACODEMON){
            bullet_sprite = world->sprites.bullet_caco;
        }
        draw_sprite(bullet_sprite, 0, bullet->pos, 2 * bullet->radius, 300 * bullet->radius, AIR);
    }
}

void draw_rockets(world_t* world) {
    player_t* player = &world->player;
    for (int i = 0; i < world->rocket_array.len; i++) {
        rocket_t* rocket = &world->rocket_array.array[i];
        draw_object(player, rocket->pos, rocket->radius, '*', FG_RED, 4, AIR);
    }
}

void draw_barrels(world_t* world) {
    player_t* player = &world->player;
    for (int i = 0; i < world->barrel_array.len; i++) {
        barrel_t* barrel = &world->barrel_array.array[i];
        draw_sprite(world->sprites.barrel, 0, barrel->pos, barrel->radius, 60, FLOOR);
    }
}

void draw_screen(world_t* world) {
    int width = olc_screen_width();
    int height = olc_screen_height();
    double d_angle = world->player.angle_of_vision / width;
    double ray_angle = world->player.angle - world->player.angle_of_vision / 2;
    double d_distance = get_config_value(kRayTraceStep);
    int row = 0;
    for (; ray_angle < world->player.angle + world->player.angle_of_vision / 2; ray_angle += d_angle) {
        double x = world->player.pos.x;
        double y = world->player.pos.y;
        double distance = 0;
        double ray_sin = sin(ray_angle);
        double ray_cos = cos(ray_angle);
        int bullet_height = 0;
        enum ray_target_e {
            wall,
            door
        } ray_target;
        while (1) {
            x += d_distance * ray_sin;
            y += d_distance * ray_cos;
            distance += d_distance;
            if (is_wall(x, y)) {
                ray_target = wall;
                break;
            }
            if (is_door(x, y)) {
                ray_target = door;
                break;
            }
        }
        int num_of_wall_sym = (int)(height * (1 / (distance)));
        if (num_of_wall_sym > height)
            num_of_wall_sym = height;
        int ceiling_level = (height - num_of_wall_sym) / 2;
        int floor_level = (height + num_of_wall_sym) / 2;
        double dx = fabs(x - round(x));
        double dy = fabs(y - round(y));
        double sprite_x;
        if (dy > dx) {
            sprite_x = fabs(y - (int)(y));
            if (ray_target == wall && is_wall(x - 0.2, y)) {
                sprite_x = 1 - sprite_x;
            }
            if (ray_target == door && is_door(x - 0.2, y)) {
                sprite_x = 1 - sprite_x;
            }
        } else {
            sprite_x = fabs(x - (int)(x));
            if (ray_target == door && is_door(x, y + 0.2)) {
                sprite_x = 1 - sprite_x;
            }
        }
        for (int i = 0; i < ceiling_level; i++) {
            world->z_buffer[row][i] = MAX_BUFF;
        }
        for (int i = ceiling_level; i < floor_level; i++) {
            double sprite_y = (i - ceiling_level) / (double)num_of_wall_sym;
            if (ray_target == wall)
                olc_draw(row, i, '#', sample_sprite_color(sprite_x, sprite_y, world->sprites.wall, 0));
            if (ray_target == door)
                olc_draw(row, i, '#', sample_sprite_color(sprite_x, sprite_y, world->sprites.door, 0));
            world->z_buffer[row][i] = distance;
        }
        for (int i = floor_level; i < height; i++) {
            olc_draw(row, i, '-', BG_DARK_GREY | FG_GREY);
            world->z_buffer[row][i] = MAX_BUFF;
        }
        row++;
    }
    draw_enemies(world);
    draw_bullets(world);
    draw_drop(world);
    draw_rockets(world);
    draw_explosions(world);
    draw_barrels(world);
}

void draw_minimap(world_t* world) {
    for (int i = 0; i < world->map_height; i++) {
        for (int j = 0; j < world->map_width; j++) {
            enum COLOR sym_col_BG;
            enum COLOR sym_col_FG;
            char sym = world->map[i][j];
            if (sym == '#') {
                sym_col_FG = FG_GREY;
                sym_col_BG = BG_GREY;
            }
            else {
                sym_col_FG = FG_BLACK;
                sym_col_BG = BG_BLACK;
            }
            olc_draw(i, world->map_width - j - 1, sym, sym_col_FG + sym_col_BG);
        }
    }

    double d_angle = world->player.angle_of_vision / olc_screen_width();
    double ray_angle = world->player.angle - world->player.angle_of_vision / 2;
    double d_distance = 0.1;
    for (; ray_angle < world->player.angle + world->player.angle_of_vision / 2; ray_angle += d_angle) {
        double x = world->player.pos.x;
        double y = world->player.pos.y;
        double ray_sin = sin(ray_angle);
        double ray_cos = cos(ray_angle);
        for (int i = 0; i < 2; i++) {
            double last_x = x;
            double last_y = y;
            while ((int)last_x == (int)x && (int)last_y == (int)y) {
                x += d_distance * ray_sin;
                y += d_distance * ray_cos;
            }
            if ((int)x >= 0 && (int)x < world->map_height && (int)y >= 0 && (int)y < world->map_width) {
                if (world->map[(int)x][(int)y] == '#') {
                    olc_draw((int)x, world->map_width - (int)y - 1, '*', FG_RED + BG_GREY);
                }
                else {
                    olc_draw((int)x, world->map_width - (int)y - 1, '*', FG_RED + BG_BLACK);
                }
            }
        }
    }

    olc_draw((int)world->player.pos.x, world->map_width - (int)world->player.pos.y - 1, '@', FG_GREEN);

    for (int i = 0; i < world->door_array.len; i++) {
        olc_draw((int)world->door_array.array[i].pos.x, world->map_width - (int)world->door_array.array[i].pos.y - 1, '8', FG_MAGENTA + BG_MAGENTA);
    }
    for (int i = 0; i < world->enemy_array.len; i++) {
        enemy_t * enemy = &world->enemy_array.array[i];
        short color = enemy->type == shooter ? FG_GREEN : FG_RED;
        olc_draw((int)enemy->pos.x, world->map_width - (int)enemy->pos.y - 1, '%', color);
    }
}

void draw_hp(world_t* world) {
    int height = olc_screen_height() / 16;
    int width = olc_screen_width() / 3;
    double hp1 = (world->player.health * width) / world->player.maxhealth;
    olc_fill(0, olc_screen_height() - height, width, olc_screen_height(), ' ', BG_RED);
    olc_fill(0, olc_screen_height() - height, (int)round(hp1), olc_screen_height(), ' ', BG_GREEN + FG_WHITE);
}
void draw_bullets_counter(world_t* world) {
    int height = olc_screen_height() / 16;
    int width = olc_screen_width() / 3;
    double bullet;
    weapon_t* weapon = get_active_weapon(world);
    double reload_animation = 0;
    if (world->weapon_list->is_reloading == 1) {
        double is_not_reloaded = 1;
        reload_animation = (world->weapon_list->time_since_last_reload / weapon->reload_delay) * (1 - weapon->magazine_bullets / weapon->max_magazine_bullets) * weapon->max_magazine_bullets;
        if (weapon->bullets + weapon->magazine_bullets < weapon->max_magazine_bullets) {
            reload_animation *= weapon->bullets + weapon->magazine_bullets / weapon->max_magazine_bullets;
        }
    }
    weapon->reload_delay;
    bullet = ((weapon->magazine_bullets + reload_animation) * width) / weapon->max_magazine_bullets;
    if (bullet > width) {
        bullet = width;
    }
    olc_fill(0, olc_screen_height() - (height * 2), (int)round(bullet), olc_screen_height() - height, ' ', BG_YELLOW);
    bullet = ((weapon->bullets) * width) / (weapon->max_magazine_bullets * 10);
    if (bullet > width) {
        bullet = width;
    }
    olc_fill(0, olc_screen_height() - (int)round(height * 2.5), (int)round(bullet), olc_screen_height() - (height * 2), ' ', BG_BLUE);
}

void draw_sprite(sprite_t* sprite, int texture_index, point_t pos, double obj_radis, double obj_height, enum PLACE_ON_SCREEN place) {
    player_t player = get_world()->player;
    screen_obj_t obj = get_object_on_screen(&player, pos, obj_radis, obj_height, place);
    if (!obj.on_screen) {
        return;
    }
    if ((obj.line_end - obj.line_start) == 0 || (obj.row_right - obj.row_left) == 0) {
        return;
    }
    int obj_screen_width = obj.row_right - obj.row_left;
    int obj_screen_height = obj.line_end - obj.line_start;

    for (int i = obj.row_left; i <= obj.row_right; i++) {
        if (i >= 0 && i <= olc_screen_width()) {
            double i_d = (double)(i - obj.row_left) / obj_screen_width;
            for (int j = obj.line_start; j <= obj.line_end; j++) {
                if (j >= 0 && j <= olc_screen_height()) {
                    double j_d = (double)(j - obj.line_start) / obj_screen_height;

                    char sym = sample_sprite_glyph(i_d, j_d, sprite, texture_index);
                    if (sym != 0 && obj.distance < get_world()->z_buffer[i][j]) {
                        olc_draw(i, j, sym, sample_sprite_color(i_d, j_d, sprite, texture_index));
                        get_world()->z_buffer[i][j] = obj.distance;
                    }
                }
            }
        }
    }
}

void draw_explosions(world_t* world) {
    for (int i = 0; i < world->explosion_array.len; i++) {
        draw_explosion(world, world->explosion_array.array[i].pos, world->explosion_array.array[i].radius, world->explosion_array.array[i].life_time);
    }
}

void draw_explosion(world_t* world, point_t pos, double radius, double life_time) {
    screen_obj_t expl = get_object_on_screen(&get_world()->player, pos, 1, 1, AIR);
    point_t expl_center;
    expl_center.x = (expl.row_left + expl.row_right) / 2;
    expl_center.y = (expl.line_start + expl.line_end) / 2;
    radius /= expl.distance;
    radius *= (1 + 250 * life_time);
    int row_start = (int)(expl_center.x - radius / 2);
    int row_end = (int)(expl_center.x + radius / 2);
    int line_start = (int)(expl_center.y - radius / 2);
    int line_end = (int)(expl_center.y + radius / 2);
    for (int i = row_start; i < row_end; i++) {
        for (int j = line_start; j < line_end; j++) {
            int state = rand() % 2;
            if (i >= 0 && j >= 0 && i < olc_screen_width() && j < olc_screen_height() && state == 1 && expl.distance < get_world()->z_buffer[i][j]) {
                const char* syms = "*%^#/|\\";
                char sym = syms[rand() % sizeof(syms)];
                olc_draw(i, j, sym, FG_RED | BG_BLACK);
                get_world()->z_buffer[i][j] = expl.distance;
            }
        }
    }
}
