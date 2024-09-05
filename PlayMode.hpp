#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	// background
	struct Background{
		uint32_t sprite_ind; //what is its index in sprite vector
		uint32_t pallete_ind; //what is its index in the pallete vector
		glm::vec2 size;
	};
	Background back;

	//player position:
	struct Player{
		glm::vec2 at = glm::vec2(0.0f);
		glm::vec2 size;
	};
	Player player;
	uint32_t points=0;

	// falling objects:
	struct Obj {
		glm::vec2 pos; //position of the food
		glm::vec2 dir; // where is it going
		glm::vec2 size; //position of the food
		uint32_t sprite_ind; //what is sprite's index in sprite vector
		uint32_t pallete_ind; //what is the index in the pallete vector
	};
	// reference to possible falling objects:
	std::vector<Obj> possible_obj;
	// list of falling objects:
	uint32_t num_obj = 5;
	std::vector<Obj> falling_obj;

	// trampoline:
	Obj trampoline;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
