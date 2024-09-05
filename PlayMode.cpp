#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

//TODO Maybe move if we want a seperate function
#include "load_save_png.hpp"
#include <iostream>
#include "data_path.hpp"

#include <random>
glm::vec2 load_sprite(PPU466 *ppu_, std::string filename, size_t tile_ind, size_t palette_ind) {
	// current plan:
	// chop up png if too big
	// find colors
	// make the tile + pallete

	std::vector< glm::u8vec4 > readin; // where we are reading in the png to
	glm::uvec2 size; // size dimensions of the png file
	load_png(data_path("../" + filename), &size, &readin, LowerLeftOrigin);

	// find the colors in the png
	PPU466::Palette colors; 
	uint32_t i = 0; // index of which color we are adding
	for(uint32_t y = 0; y < readin.size(); ++y){
		if (std::find(colors.begin(), colors.begin()+i, readin[y]) == colors.begin()+i){
			if (i==4){ 
				printf("Hit max color amount at ");
				std::cout<<filename<<std::endl;
				break; // currently just cutting it off at 4
			} 			
			colors[i] = readin[y];
			i++;
		}
	}
	ppu_->palette_table[palette_ind] = colors;

	// after loading, resize into a 2d list of pallete indices based on size/colors
	//in case we need multiple tiles, offset keep track of which one
	uint32_t offset = 0; 
	glm::vec2 out_size = glm::vec2((uint32_t)(std::floor((size[0]+7)/8)),(uint32_t)(std::floor((size[1]+7)/8)));
	for(uint32_t row = 0; row < size[1]; row+=8){
		for(uint32_t col = 0; col < size[0]; col+=8){
			// we want 8x8 sized tiles
			PPU466::Tile t;
			for(uint32_t r = row; r < row+8; ++r){
				t.bit0[r-row] =0;
				t.bit1[r-row] = 0;
				for(uint32_t c = col; c < col+8; ++c){
					uint32_t index = std::find(colors.begin(),colors.end(),readin[r*size[0]+c])-colors.begin(); 
					if (index>=4){index = 0;}
					t.bit0[r-row] |= (index&1)<<(c-col);
					t.bit1[r-row] |= ((index>>1)&1)<<(c-col);
				}
			}
			if(ppu_->tile_table.size()<=tile_ind+offset){
				printf("HEY WE ARE OUT OF SPACE!\n");
			}
			ppu_->tile_table[tile_ind+offset] = t;
			offset++;
		}
	}
	return out_size; // return cut up size (orig size divided into 8x8 tiles)
}

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository. 
	
	for(size_t i = 0; i<ppu.palette_table.size();i++){
		// zero everything out
		ppu.palette_table[i] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		};
	}
	uint32_t tile_ind=0;
	uint32_t pallete_ind=0;
	// loading player tiles
	player.size = load_sprite(&ppu,"data/basket.png",tile_ind,pallete_ind);
	tile_ind+=player.size[0]*player.size[1];
	pallete_ind++;

	// loading trampoline
	trampoline.size = load_sprite(&ppu,"data/trampoline.png",tile_ind,pallete_ind);
	trampoline.sprite_ind = tile_ind;
	trampoline.pallete_ind = pallete_ind;
	trampoline.pos = glm::vec2(std::rand()% (PPU466::BackgroundWidth*4),std::rand()%80 + PPU466::BackgroundHeight*2); //random position
	tile_ind+=trampoline.size[0]*trampoline.size[1];
	pallete_ind++;

	// loading background tiles
	back.size = load_sprite(&ppu,"data/background.png",tile_ind,pallete_ind);
	back.sprite_ind = tile_ind;
	back.pallete_ind = pallete_ind;
	tile_ind+=back.size[0]*back.size[1];
	pallete_ind++;

	// loading falling tiles and add to the possible object lists
	std::vector<std::string> falling_paths = {"data/strawberry.png","data/carrot.png","data/cabbage.png"};
	for(std::string str:falling_paths){
		static Obj food;
		food.size = load_sprite(&ppu,str,tile_ind,pallete_ind);
		food.sprite_ind = tile_ind;
		food.pallete_ind = pallete_ind;
		possible_obj.emplace_back(food);

		tile_ind+=food.size[0]*food.size[1];
		pallete_ind++;
	}

	// initializing some of the falling foods
	for(size_t i = 0; i<num_obj; i++){
		static Obj food;
		Obj ref = possible_obj[std::rand()%possible_obj.size()];
		food.sprite_ind = ref.sprite_ind;
		food.pallete_ind = ref.pallete_ind;
		food.size = ref.size;
		food.pos = glm::vec2(std::rand()% (PPU466::BackgroundWidth*4),PPU466::BackgroundHeight*4); //random position
		falling_obj.emplace_back(food);
	}
	assert(falling_obj.size() == num_obj);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {

	constexpr float PlayerSpeed = 50.0f;
	if (left.pressed) player.at.x -= PlayerSpeed * elapsed;
	if (right.pressed) player.at.x += PlayerSpeed * elapsed;
	
	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// falling foods
	constexpr float FallingSpeed = 40.0f;
	for(size_t i = 0; i<falling_obj.size(); i++){
		falling_obj[i].pos += falling_obj[i].dir * elapsed;
		falling_obj[i].dir.y -=FallingSpeed * elapsed;

		if (falling_obj[i].pos.y <= player.at.y + player.size.y*8){
			// check if in basket
			if (falling_obj[i].pos.x>= std::max(0.0f,player.at.x-falling_obj[i].size.x*8 )&& falling_obj[i].pos.x <=player.at.x + player.size.x*8){
				points +=10;  // add points
				printf("Collected Points: %d\n",points);
				// if we get to this point, could also check if the item was a power item to move the trampoline
			}
			// remove/ move back to top with random image -- reset the image index + size
			falling_obj[i].pos.x = std::rand() % (PPU466::BackgroundWidth * 4);
			falling_obj[i].pos.y = (PPU466::BackgroundHeight * 4);
			falling_obj[i].dir = glm::vec2(0,0);
			
			Obj ref = possible_obj[std::rand()%possible_obj.size()];
			falling_obj[i].sprite_ind = ref.sprite_ind;
			falling_obj[i].pallete_ind = ref.pallete_ind;
			falling_obj[i].size = ref.size;
		}
		// hitting trampoline
		else if(falling_obj[i].pos.y <= trampoline.pos.y + trampoline.size.y*8 && 
				falling_obj[i].pos.y + falling_obj[i].size.y*8 >= trampoline.pos.y&& 
				falling_obj[i].pos.x <= trampoline.pos.x + trampoline.size.x*8 && 
				falling_obj[i].pos.x + falling_obj[i].size.x*8 >= trampoline.pos.x){
			// add bounce energy
			falling_obj[i].dir = glm::vec2(15.0f,60.0f); 
			// case on where it is in the board to bounce a visible direction
			if(falling_obj[i].pos.x >= PPU466::BackgroundWidth * 2){ 
				falling_obj[i].dir.x = -15.0f;
			}
		}
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	ppu.background_color = glm::u8vec4(0,0,0,0xff);

	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			// offsets
			uint32_t y_off = (y % (uint32_t)back.size.y);
			uint32_t x_off = (x % (uint32_t)back.size.x);
			ppu.background[x+PPU466::BackgroundWidth*y] = 
			(uint32_t)(back.sprite_ind + y_off*back.size.y+x_off)| (back.pallete_ind<<8);
		}
	}

	//player sprite:
	size_t sprite_ind=0;
	for(size_t r =0 ;r<player.size[1];r++){
		for(size_t c=0;c<player.size[0];c++){
			ppu.sprites[sprite_ind].x = int8_t(player.at.x) + c*8;
			ppu.sprites[sprite_ind].y = int8_t(player.at.y) + r*8;
			ppu.sprites[sprite_ind].index = 0 + r*player.size[0]+c;
			ppu.sprites[sprite_ind].attributes = 0;
			sprite_ind++;
	}}

	// trampoline
	for(size_t r =0 ;r<trampoline.size[1];r++){
		for(size_t c=0;c<trampoline.size[0];c++){
			ppu.sprites[sprite_ind].x = int8_t(trampoline.pos.x) + c*8;
			ppu.sprites[sprite_ind].y = int8_t(trampoline.pos.y) + r*8;
			ppu.sprites[sprite_ind].index = trampoline.sprite_ind + r*trampoline.size[0]+c;
			ppu.sprites[sprite_ind].attributes = trampoline.pallete_ind;
			sprite_ind++;
	}}
	// for each falling food:
	for(Obj food:falling_obj){
		// printf("food %f %f",food.size[0],food.size[1]);
		for(size_t r =0 ;r<food.size[1];r++){
			for(size_t c=0;c<food.size[0];c++){
				ppu.sprites[sprite_ind].x = int8_t(food.pos.x) + c*8;
				ppu.sprites[sprite_ind].y = int8_t(food.pos.y) + r*8;
				ppu.sprites[sprite_ind].index = food.sprite_ind + r*food.size[0] + c;
				ppu.sprites[sprite_ind].attributes = food.pallete_ind;
				sprite_ind++;
			}
		}
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
