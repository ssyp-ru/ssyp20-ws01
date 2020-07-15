#include "../third_party/olc/olc.h"

#include "world_object.h"

#include "render.h"

#include <math.h>

void draw_enemies(int row, double distance) {
    int bullet_height = 60 / distance;
    for (int i = olc_screen_height() / 2 - bullet_height + 0.5; i < olc_screen_height() / 2 + bullet_height + 0.5; i++)
        olc_draw(row, i, '%', FG_BLUE);
}

void draw_bullet(int row, double distance) {
    int bullet_height = 4 / distance;
    for (int i = olc_screen_height() / 2 - bullet_height + 0.5; i < olc_screen_height() / 2 + bullet_height + 0.5; i++)
        olc_draw(row, i, '*', FG_RED);
}

void draw_screen(world_t* world) {
    int width = olc_screen_width();
    int height = olc_screen_height();
    int threshold1 = 125;
    int threshold2 = 140;
    double d_angle = world->player.angle_of_vision / width;
    double ray_angle = world->player.angle - world->player.angle_of_vision / 2;
    double d_distance = 0.01;
    int row = 0;
    for (; ray_angle < world->player.angle + world->player.angle_of_vision / 2; ray_angle += d_angle) {
        double x = world->player.pos.x;
        double y = world->player.pos.y;
        double distance = 0;
        double ray_sin = sin(ray_angle);
        double ray_cos = cos(ray_angle);
        while (!is_wall(x, y)) {
            x += d_distance * ray_sin;
            y += d_distance * ray_cos;
            distance += d_distance;
        }
        int num_of_wall_sym = height * (1 / (distance));
        if (num_of_wall_sym > height)
            num_of_wall_sym = height;

		int ceiling_level = (height - num_of_wall_sym) / 2;
		int floor_level = (height + num_of_wall_sym) / 2;
		char sym = '#';
		for (int i = ceiling_level; i < floor_level; i++) {
			olc_draw(row, i, sym, FG_WHITE);
		}
		for (int i = floor_level; i < height; i++) {
			if (i < threshold1) {
				olc_draw(row, i, '-', FG_GREY);
			}
			else if (i < threshold2) {
				olc_draw(row, i, 'x', FG_GREY);
			}
			else {
				olc_draw(row, i, 'X', FG_GREY);
			}
		}

        x = world->player.pos.x;
        y = world->player.pos.y;
        distance = 0;
        while (!is_wall(x, y)) {
            x += d_distance * ray_sin;
            y += d_distance * ray_cos;
            distance += d_distance;
            if (is_enemy(x, y, NULL)) {
                draw_enemies(row, distance);
            }
        }
        x = world->player.pos.x;
        y = world->player.pos.y;
        distance = 0;
        while (!is_wall(x, y)) {
            x += d_distance * ray_sin;
            y += d_distance * ray_cos;
            distance += d_distance;
            if (is_bullet(x, y)) {
                draw_bullet(row, distance);
            }
        }
		row++;
	}
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
    for (int i = 0; i < world->enemy_array.len; i++) {
        olc_draw((int)world->enemy_array.array[i].pos.x, world->map_width - (int)world->enemy_array.array[i].pos.y - 1, '%', FG_GREEN);
    }
}
