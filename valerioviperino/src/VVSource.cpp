#include "VVSource.h"
#include "vv_math.h"

//--------------------------------------------------------------
// initialise all the vars required by the various checkpoints
//--------------------------------------------------------------
void VVSource::init_vars(){

    app_should_quit = false;

    // initialise time at the start of source
    show_start_time = ofGetElapsedTimef();
    if (show_start_time >= EXHIBITION_TIME) show_start_time = ofGetElapsedTimef() - current_show_time;

    // reference_image.allocate(990, 585, OF_IMAGE_COLOR_ALPHA);
    reference_image.load("accordion_reference_matching_bg.png");

    CHECKPOINT_1 = false;
    // used to keep timing in the intro
    intro_time_multiplier = 1;
    intro_started = false;
    for (int i = 0; i < 6; i++){
        rectangle_triggers[i] = false;
    }

    // checkpoint 2
    CHECKPOINT_2 = false;
    lines_started = false;
    rects_started = false;

    // checkpoint 3
    CHECKPOINT_3 = false;
    center_rect_size = ofVec2f(fbo->getWidth(), fbo->getHeight());
    bars_started = false;
    show_left_ellipse = false;
    white_quads_started = false;
    white_quads_ended = false;
    black_quads_ended = false;
    // find the biggest quad that if tiled covers a 1/4 of the fbo
    quad_size = find_max_square_to_evenly_fit_rect(int(fbo->getWidth() / 4), int(fbo->getHeight()));

    // checkpoint 4
    CHECKPOINT_4 = false;
    num_rects_h_1 = 0;
    num_rects_h_2 = 0;
    num_rects_h_3 = 0;
    num_rects_h_4 = 0;
    bg_started_fade = false;
    coloured_quads_started = false;
    coloured_lines_started = false;
}

//--------------------------------------------------------------
void VVSource::setup(){
	// Give our source a decent name
    name = "VV Source";

	// Allocate our FBO source
    allocate(1000, 600);
    // this->allocate(990, 585, GL_RGBA, 8)

    // clear fbo
    this->beginFbo();
        ofClear(255,0,0);
    this->endFbo();

    init_vars();

    // making it bigger since the projector is low res
    // and the finer details and shapes really get lost
    quad_size = fbo->getWidth() / 40;
    
    // how many quads fit our fbo surface
    int num_x_quads = fbo->getWidth() / quad_size;
    int num_y_quads = fbo->getHeight() / quad_size;
    
    // fill color vector for the final quads with hue arranged colors
    for (int i = 0; i < num_x_quads*num_y_quads; i++){
        ofColor color;
        float hue = ofMap(i, 0, num_x_quads*num_y_quads, 0, 255);
        color.setHsb(hue, 220, 120);

        final_quads_colors.push_back(color);
    }
}

//--------------------------------------------------------------
void VVSource::setName(string _name){
    name = _name;
}

// Don't do any drawing here
void VVSource::update(){
    current_show_time = (ofGetElapsedTimef() - show_start_time);
    // cout << endl;
    // cout << "ofGetElapsedTimef(): " << ofGetElapsedTimef() << endl;
    // cout << "current_show_time  : " << current_show_time << endl;
    // cout << "show_start_time    : " << show_start_time << endl;
}

void VVSource::reset(){
    ofClear(0); // uncomment if you want canvas to be reset on the buffer when fbo source is called again
    cout << "vv, called reset" << endl;
    init_vars();
}

// No need to take care of fbo.begin() and fbo.end() here.
// All within draw() is being rendered into fbo;
void VVSource::draw(){

    ofPushStyle();
    ofClear(0); // remove if you never want to update the background
    // ofBackground(0);

    ofSetCircleResolution(20);
    ofFill();

    // draw flashing intro rectangles
    if (!CHECKPOINT_1){
        drawFlashingIntro(intro_time_multiplier, current_show_time);
    }
    // moving lines and quads
    else if (CHECKPOINT_1 && !CHECKPOINT_2){
        drawMovingLines(current_show_time);
    }
    // fading quads and change of 2D plane
    else if (CHECKPOINT_2 && !CHECKPOINT_3){
        drawFadingQuads(current_show_time);
    }
    // colors!
    else if (CHECKPOINT_3 && !CHECKPOINT_4){
        drawColouredLines(current_show_time);
    }

    // ofEnableAlphaBlending();
    // reference_image.draw(0, 0);
    // ofDisableAlphaBlending();

    // if 'd' is pressed
    if (show_calibration_grid){
        drawCalibrationGrid(16);
    }

    ofPopStyle();
}
//--------------------------------------------------------------
// 0
// draw a grid, useful for calibration
//--------------------------------------------------------------
void VVSource::drawCalibrationGrid(int numOfLines){
    
    for (int i = 0; i < numOfLines; i++){
        ofVec2f increment(i * fbo->getWidth() / numOfLines, i * fbo->getHeight() / numOfLines);
        ofSetColor(0, 255, 0);
        // v
        ofDrawLine(increment.x, 0, increment.x, fbo->getHeight());
        // h
        ofDrawLine(0, increment.y, fbo->getWidth(), increment.y);
        ofSetColor(255, 0, 0);
    }
}

//--------------------------------------------------------------
// 1
//--------------------------------------------------------------
void VVSource::drawFlashingIntro(int & time_multiplier, float currentShowTime){

    int colors[] = {0, 0, 0, 0};
    float durations[7] = {4, 4, 2, 2, 2, 1, 2}; // seconds

    if (!intro_started){
        intro_started = true;
        intro_start_time = currentShowTime;
        cout << "----------------" << endl;
        cout << "   VV STARTED" << endl;
        cout << "----------------" << endl;

        for (int i = 0; i < 7; i++){
            if (i > 0){
                intro_checkpoints[i] = durations[i] + intro_checkpoints[i-1];
            }
            else {
                intro_checkpoints[i] = 0;
            }
            // cout << "current_checkpoint : " << i << " : " << intro_checkpoints[i] << endl;
        }
    }

    // cout << "time multiplier : " << time_multiplier << endl;

    ofPushStyle();
    
    bool show_rects = true;

    // compute local start time
    float current_time = (currentShowTime - intro_start_time);

    // cout << endl << "intro_start_time: " << intro_start_time << endl;
    // cout << "current_time: " << current_time << endl;

    // change rect color based on current time
    if ((current_time >= intro_checkpoints[0]) && (current_time < intro_checkpoints[1]) && (rectangle_triggers[0] == false)){
        time_multiplier = 2;
        rectangle_triggers[0] = true;
        // cout << "increased time:" << time_multiplier << "x" << endl;
    }
    else if ((current_time >= intro_checkpoints[1] && current_time < intro_checkpoints[2]) && (rectangle_triggers[1] == false)){
        time_multiplier = 4;
        rectangle_triggers[1] = true;
        // cout << "increased time:" << time_multiplier << "x" << endl;
    }
    else if ((current_time >= intro_checkpoints[2] && current_time < intro_checkpoints[3]) && rectangle_triggers[2] == false){
        time_multiplier = 8;
        rectangle_triggers[2] = true;
        // cout << "increased time:" << time_multiplier << "x" << endl;
    }
    else if ((current_time >= intro_checkpoints[3] && current_time < intro_checkpoints[4]) && (rectangle_triggers[3] == false)){
        time_multiplier = 20;
        rectangle_triggers[3] = true;
        // cout << "increased time:" << time_multiplier << "x" << endl;
    }
    else if ((current_time >= intro_checkpoints[4] && current_time < intro_checkpoints[5]) && (rectangle_triggers[4] == false)){
        time_multiplier = 100;
        rectangle_triggers[4] = true;
        // cout << "increased time:" << time_multiplier << "x" << endl;
    }
    else if (current_time >= intro_checkpoints[6] && rectangle_triggers[5] == false){
        show_rects = false;
    }
    
    if (show_rects){

        ofVec2f size(fbo->getWidth()/4, fbo->getHeight());
        ofVec2f pos(0, 0);
        
        current_time *= time_multiplier;
        
        colors[0] = (int(current_time) % 4 == 0) ? 255 : 0;
        colors[1] = (int(current_time) % 4 == 1) ? 255 : 0;
        colors[2] = (int(current_time) % 4 == 2) ? 255 : 0;
        colors[3] = (int(current_time) % 4 == 3) ? 255 : 0;
        
        ofSetColor(colors[0]);
        ofDrawRectangle(pos.x, pos.y, size.x, size.y);

        ofSetColor(colors[1]);
        ofDrawRectangle(1 * size.x, pos.y, size.x, size.y);

        ofSetColor(colors[2]);
        ofDrawRectangle(2 * size.x, pos.y, size.x, size.y);

        ofSetColor(colors[3]);
        ofDrawRectangle(3 * size.x, pos.y, size.x, size.y);
    }
    else {
        CHECKPOINT_1 = true;
    }

    ofPopStyle();
}

//--------------------------------------------------------------
// 2
//--------------------------------------------------------------
void VVSource::drawMovingLines(float currentShowTime){

    // save start time of animation
    if (!lines_started){
        lines_start_time = currentShowTime;
        lines_started = true;
    }
    bool lines_reached_end = false;
    int num_of_lines = fbo->getWidth() / 20;
    float line_spacing = fbo->getWidth() / num_of_lines;

    ofPushStyle();

    float checkpoints[4] = {0, 0.1, 0.2, 0.3}; // seconds
    float lines_checkpoints[3] = {1, 2.5, 11};
    float rect_checkpoints[3] = {13, 14};

    // compute "local" time since the first time this function was called
    float current_time = currentShowTime - lines_start_time;

    // draw two flashes of the lines grid
    if ((current_time > checkpoints[0] && current_time < checkpoints[1]) ||
        (current_time > checkpoints[2] && current_time < checkpoints[3])){
        
        int num_of_lines = fbo->getWidth() / 20;

        int color = (int(current_time) % 20 == 0) ? 255 : 0; 
        ofSetColor(color);

        for (int i = 0; i < num_of_lines; i++){
            ofVec2f increment(i * fbo->getWidth() / num_of_lines, i * fbo->getHeight() / num_of_lines);
            // v
            ofDrawLine(increment.x, 0, increment.x, fbo->getHeight());
            // h
            ofDrawLine(0, increment.y, fbo->getWidth(), increment.y);
        }
    }
    // lines animation
    // growing lines + growing rectangles
    else if (current_time > lines_checkpoints[0] && current_time < lines_checkpoints[2]){

        ofSetColor(255);

        float time_multiplier = 0.15;
        float lines_width;

        // vertical lines
        for (int i = 0; i < fbo->getWidth(); i+=line_spacing){
            float time_offset = (i / line_spacing) * time_multiplier;
            // set width based one line index
            lines_width = ofMap(i, 0, fbo->getWidth(), 1, 7, true);
            ofSetLineWidth(lines_width);
            ofVec2f start_pos(i, fbo->getHeight());
            ofVec2f end_pos(i, ofMap(current_time, lines_checkpoints[0] + time_offset, lines_checkpoints[1] + time_offset, fbo->getHeight(), 0, true));
            ofDrawLine(start_pos, end_pos);
        }

        // horizontal lines
        time_multiplier = 0.25;
        for (int i = 0; i < fbo->getHeight(); i+=line_spacing){
            float time_offset = (i / line_spacing) * time_multiplier;
            // set width
            lines_width = ofMap(i, 0, fbo->getHeight(), 7, 1, true);
            ofSetLineWidth(lines_width);
            ofVec2f start_pos(0, i);
            ofVec2f end_pos(ofMap(current_time, lines_checkpoints[0] + time_offset, lines_checkpoints[1] + time_offset, 0, fbo->getWidth(), true), i);
            ofDrawLine(start_pos, end_pos);

            // if we are at the end of the animation
            if (i >= fbo->getHeight()-line_spacing && current_time >= (lines_checkpoints[1] + time_offset) + 1){
                lines_reached_end = true;
            }
        }
        if (lines_reached_end){

            // save start time for the next one: rectangles
            if (!rects_started){
                rects_start_time = current_time;
                rects_started = true;
                ofSetLineWidth(2);
            }
        }
    }
    // rects growing for both sides
    else if (current_time > lines_checkpoints[0] && current_time < rect_checkpoints[1] && rects_started){

        ofSetColor(255);
        ofFill();

        // flash lines in the background
        if (current_time < rect_checkpoints[0]){
            // vertical
            if (ofGetFrameNum() % 10 == 0){
                for (int i = 0; i < fbo->getWidth(); i+=line_spacing){
                    ofVec2f start_pos(i, fbo->getHeight());
                    ofVec2f end_pos(i, 0);
                    ofDrawLine(start_pos, end_pos);
                }
            }
            // horizontal
            if (ofGetFrameNum() % 20 == 0){
                for (int i = 0; i < fbo->getHeight(); i+=line_spacing){
                    ofVec2f start_pos(0, i);
                    ofVec2f end_pos(fbo->getWidth(), i);
                    ofDrawLine(start_pos, end_pos);
                }
            }
        }

        int target_horizontal_rects_num = fbo->getWidth() / 20;
        int target_vertical_rects_num = fbo->getHeight() / 20;
        int current_horizontal_rects_num = ofMap(current_time, rects_start_time, rects_start_time+2, 0, target_horizontal_rects_num);
        float rect_size = fbo->getWidth() / target_horizontal_rects_num;
        
        // forward animation
        for (int y = 0; y < target_vertical_rects_num*2; y+=2){
            for (int i = 0; i < current_horizontal_rects_num - y; i++){
                ofVec2f pos(i * rect_size, y * rect_size);
                ofDrawRectangle(pos.x, pos.y, rect_size, rect_size);
            }
        }
        // backward animation
        for (int y = 1; y < target_vertical_rects_num*2; y+=2){
            for (int i = 0; i < current_horizontal_rects_num - y; i++){
                ofVec2f pos((target_horizontal_rects_num - i) * rect_size, y * rect_size);
                ofDrawRectangle(pos.x, pos.y, rect_size, rect_size);
            }
        }
    }
    // final rect animation with scale shrinking
    else if (current_time >= rect_checkpoints[1]){

        int colors[4] = {255, 255, 255, 255};

        colors[0] = (current_time > rect_checkpoints[1] + 1) == true ? 0 : 255;
        colors[1] = (current_time > rect_checkpoints[1] + 1.5) == true ? 0 : 255;
        colors[2] = (current_time > rect_checkpoints[1] + 2.0) == true ? 0 : 255;
        colors[3] = (current_time > rect_checkpoints[1] + 2.5) == true ? 0 : 255;

        ofVec2f size(fbo->getWidth(), fbo->getHeight());

        ofPushMatrix();
        
        ofTranslate(fbo->getWidth()/2, fbo->getHeight()/2);

        float scale_factor = ofMap(current_time, rect_checkpoints[1] + 1, rect_checkpoints[1] + 2, 1, 0.01, true);
        ofScale(1, scale_factor, 1);

        ofDrawRectangle(-size.x/2, -size.y/2, size.x, size.y);

        // cout << "scale factor: " << scale_factor << endl;

        if (scale_factor <= 0.02){
            CHECKPOINT_2 = true;
        }
        
        ofPopMatrix();
    }
    
    ofPopStyle();
}

//--------------------------------------------------------------
// 3
// draw the quads fadeing in while rotating
// and the lines animated from bottom to top
//--------------------------------------------------------------
void VVSource::drawFadingQuads(float currentShowTime){
    
    // save the start time
    if (!bars_started){
        bars_start_time = currentShowTime;
        bars_started = true;
    }

    // compute "local" time since the first time this function was called
    float current_time = currentShowTime - bars_start_time;
    float checkpoints[4] = {0, 1, 3, 22};
    
    ofPushStyle();

    // this if cascade makes different things happen based on certain bools
    // 1. scale and rotate the square into the 2 center panels
    if (current_time > checkpoints[0] && current_time < checkpoints[1]){
        ofPushMatrix();
        ofTranslate(fbo->getWidth()/2, fbo->getHeight()/2);
        
        ofRotateZ(ofMap(current_time, checkpoints[0], checkpoints[1], 0, 90, true));
        ofScale(1, 0.02, 1);
        
        ofVec2f size(fbo->getWidth(), fbo->getHeight());

        ofDrawRectangle(-size.x/2, -size.y/2, size.x, size.y);
        ofPopMatrix();
    }
    // 2. quads fade in animation
    // first black to white, then white to black
    else if (current_time > checkpoints[1] && !black_quads_ended){
        
        // replicate the center rectangle that we left in the previous checkpoint
        if (!white_quads_ended){
            ofPushMatrix();
                ofTranslate(fbo->getWidth()/2, fbo->getHeight()/2);
                
                ofRotateZ(90);
                // animate the scale
                float scale_factor = ofMap(current_time, checkpoints[1], checkpoints[2], 0.01, 0.84f, true);
                ofScale(1, scale_factor, 1);

                ofDrawRectangle(-center_rect_size.x/2, -center_rect_size.y/2, center_rect_size.x, center_rect_size.y);
            ofPopMatrix();
            
            // when the center animation is finished, start the quads one
            if (scale_factor >= 0.84f && !white_quads_started){
                white_quads_started = true;
                white_quads_start_time = current_time;
            }
        }

        ofPushMatrix();

        // vars required for the next stage
        float quads_color;
        float time_offset = 0.0f;
        float duration = 0.9f;
        float x_time_offset_multiplier = 0.012; // used to delay quads on different cols
        float y_time_offset_multiplier = 0.016; // used to delay quads on different rows
        
        // slowly fade in + rotation of the quads
        // draw them on first panel from the left and first from the right
        // repeat until animation is done
        if (white_quads_started && !white_quads_ended){
                
            // animate the LEFT PANEL (black --> white)
            for (int x = 0; x <= fbo->getWidth()/4; x+=quad_size){
                for (int y = fbo->getHeight(); y >= 0; y-=quad_size){
                    
                    // how much delay there is from a column to another
                    float y_time_shift = ofMap(y, fbo->getHeight(), 0, 0, fbo->getHeight());
                    
                    time_offset = (x * x_time_offset_multiplier) + (y_time_shift * y_time_offset_multiplier);

                    // animate color, scale, rotation
                    float start_time = white_quads_start_time + time_offset;
                    float end_time = white_quads_start_time + time_offset + duration;
                    float end_time_color = white_quads_start_time + time_offset + (duration * 1.3);

                    quads_color = ofMap(current_time, start_time, end_time_color, 0, 255, true);
                    float scale_animated = ofMap(current_time, start_time, end_time, 0.001, 1, true);
                    float rotation_animated = ofMap(current_time, start_time, end_time, 90, 0, true);

                    ofPushMatrix();

                        ofTranslate(x, y, 0);
                        ofRotateX(rotation_animated);

                        ofSetColor(quads_color);
                        ofDrawRectangle(-quad_size, 0, quad_size * scale_animated, quad_size * scale_animated);

                        // check when we're done
                        if (x >= fbo->getWidth()/4 - quad_size && y <= quad_size){
                            if (quads_color == 255 && scale_animated == 1 && rotation_animated == 0){
                                white_quads_ended = true;
                                black_quads_start_time = current_time;
                            }
                        }
                    ofPopMatrix();
                }
            }
            
            // move to the other panel
            ofTranslate(fbo->getWidth() * 3/4, 0, 0);
            // animate the RIGHT PANEL (black --> white)
            for (int x = fbo->getWidth()/4; x >= 0; x-=quad_size){
                for (int y = fbo->getHeight(); y >= 0; y-=quad_size){
                    
                    // make quads appear slowly from bottom right
                    float y_time_shift = ofMap(y, fbo->getHeight(), 0, 0, fbo->getHeight());
                    float x_time_shift = ofMap(x, fbo->getWidth()/4, 0, 0, fbo->getWidth()/4);
                    time_offset = (x_time_shift * x_time_offset_multiplier) + (y_time_shift * y_time_offset_multiplier);
                    // animate color, scale, rotation
                    quads_color = ofMap(current_time, white_quads_start_time + time_offset, white_quads_start_time + time_offset + (duration * 1.3), 0, 255, true);
                    float scale_animated = ofMap(current_time, white_quads_start_time + time_offset, white_quads_start_time + time_offset + duration, 0.001, 1, true);
                    float rotation_animated = ofMap(current_time, white_quads_start_time + time_offset, white_quads_start_time + time_offset + duration, 90, 0, true);

                    ofPushMatrix();

                        ofTranslate(x, y, 0);
                        ofRotateX(rotation_animated);

                        ofSetColor(quads_color);
                        ofDrawRectangle(0, 0, quad_size * scale_animated, quad_size * scale_animated);

                    ofPopMatrix();
                }
            }
            // ofPopMatrix();
        }
        ofPopMatrix();

        ofPushMatrix();
        
        // when the white quads animation is ended
        if (white_quads_ended){

            // cout << "elapsed time: " << current_time - white_quads_start_time << endl;

            ofSetColor(255);
            duration = 0.5f;
            // ofDrawRectangle(0, 0, fbo->getWidth(), fbo->getHeight());

            // fade out to black using the same technique
            // but now from top to bottom
            for (int y = 0; y < fbo->getHeight(); y+=quad_size){
                for (int x = 0; x < fbo->getWidth(); x+=quad_size){
                    
                    // time_offset = (x * x_time_offset_multiplier) * (y * y_time_offset_multiplier) * 0.17;
                    time_offset = (x * x_time_offset_multiplier) * (y * y_time_offset_multiplier) * 0.17;
                    
                    // time_offset = (y * y_time_offset_multiplier);
                    // animate color, scale, rotation
                    float scale_animated = ofMap(current_time, black_quads_start_time + time_offset, black_quads_start_time + time_offset + duration, 1, 0.0f, true);
                    float rotation_animated = ofMap(current_time, black_quads_start_time + time_offset, black_quads_start_time + time_offset + duration, 0, 360, true);

                    quads_color = ofMap(current_time, black_quads_start_time + time_offset, black_quads_start_time + time_offset + (duration * 2.0), 255, 0, true);
                    ofColor current_color = (quads_color, quads_color, quads_color, quads_color);

                    ofPushMatrix();
                        ofTranslate(x, y, 0);
                        
                        ofRotateY(rotation_animated);

                        ofSetColor(current_color);
                        ofDrawRectangle(0, 0, quad_size * scale_animated, quad_size * scale_animated);

                    ofPopMatrix();
                    
                    // check when we're done
                    if (x >= fbo->getWidth() - quad_size && y >= fbo->getHeight() - quad_size){
                        if (quads_color == 0 && scale_animated == 0.0f && rotation_animated == 360){
                            black_quads_ended = true;
                            v_lines_start_time = current_time;
                            break;
                        }
                    }
                }
            }
        }
        ofPopMatrix();

        ofPopMatrix();
    }
    // 3. animated lines from bottom to up
    // start only when the fade to black quads animation is ended
    else if (black_quads_ended){

        ofSetLineWidth(8);
        ofDrawLine(0, fbo->getHeight(), 0, 0);
        // animate lines coming up from the bottom
        float v_lines_duration = 2; 
        float animated_y_head = ofMap(current_time, v_lines_start_time, v_lines_start_time + v_lines_duration, fbo->getHeight(), 0, true);
        float animated_y_tail = fbo->getHeight();
        ofColor line_color;

        float rotate_amount; // animated rotation around X axis
        float move_amount; // animated movement along Y axis

        // 3.2 break the 2D barriers! rotate around X axis
        if (current_time >= v_lines_start_time + v_lines_duration){

            float movement_start_time = v_lines_start_time + v_lines_duration + 1;
            float movement_end_time = movement_start_time + 2;

            rotate_amount = ofMap(current_time, movement_start_time, movement_end_time, 0, 43, true);
            move_amount = ofMap(current_time, movement_start_time, movement_end_time, 0, fbo->getHeight()/2, true);
            ofTranslate(0, move_amount, 0);
            ofRotateX(rotate_amount);
            
            // animate lines growing on the fake z axis
            if (current_time >= movement_end_time){
                float time_offset = 1;
                float z_movement_start_time = movement_end_time + time_offset;
                float z_movement_end_time = z_movement_start_time + 8;
                
                animated_y_head = ofMap(current_time, z_movement_start_time, z_movement_end_time, 0, -fbo->getHeight()*1000, true);
                animated_y_tail = ofMap(current_time, z_movement_start_time, z_movement_end_time, fbo->getHeight(), -fbo->getHeight()*100, true);
            }
        }
        // 3.1 draw the lines
        int num_of_lines = 16;
        float line_spacing_x = fbo->getWidth() / num_of_lines;
        for (int i = 0; i < num_of_lines; i++){
            
            // set hue
            if (rotate_amount > 40){
                float line_hue = ofMap(i, 0, num_of_lines, 0, 255);
                line_color.setHsb(line_hue, 180, 100, 255);
                ofSetColor(line_color);
            }
            
            ofDrawLine(i * line_spacing_x, animated_y_tail, i * line_spacing_x, animated_y_head);
        }

        // go to the next function!
        if(animated_y_tail < -10000){
            CHECKPOINT_3 = true;
        }
    }
    ofPopStyle();
}

//--------------------------------------------------------------
// 4
//--------------------------------------------------------------
void VVSource::drawColouredLines(float currentShowTime){

    // save the start time
    if (!coloured_lines_started){
        coloured_lines_start_time = currentShowTime;
        coloured_lines_started = true;
    }

    // compute "local" time since the first time this function was called
    float current_time = currentShowTime - coloured_lines_start_time;

    ofPushStyle();

    // max sizes of the rectangles
    float max_x_rect_size = 6;
    float max_y_rect_size = 12;
    // actual sizes
    float size_x;
    float size_y;
    
    int lines_y_step = fbo->getHeight() / max_y_rect_size;

    // colors
    float saturation = 50;
    float hue;
    float darkness = 198;
    ofColor color;

    ofPushMatrix();
    
    ofEnableBlendMode(OF_BLENDMODE_ADD);

    // slowly add more rectangles
    if (ofGetFrameNum() % 60 == 0) num_rects_h_1++;
    if (num_rects_h_1 > 10 && ofGetFrameNum() % 40 == 1) num_rects_h_2++;
    if (num_rects_h_2 > 10 && ofGetFrameNum() % 30 == 0) num_rects_h_3++;
    if (num_rects_h_3 > 10 && ofGetFrameNum() % 20 == 0) num_rects_h_4++;

    ofPushMatrix();

    // this if cascade makes different things happen based on certain bools and conditions
    // draw different coloured lines
    // horizontal
    if (!bg_started_fade){
        
        if (current_time > 0){
            for (int i = 0; i < num_rects_h_1; i++){
                // size
                size_x = fbo->getWidth()/4;
                size_y = ofMap(ofSignedNoise(i, current_time*3), -1, 1, 2, max_y_rect_size);
                // colors
                hue = ofMap(i * lines_y_step, 0, fbo->getHeight(), 0, 255);
                color.setHsb(hue, saturation, darkness, 255);
                ofSetColor(color);
                // ofDrawRectangle(0, i * lines_y_step + (sin(ofSignedNoise(current_time * 0.005)) * 20), size_x, size_y);
                ofDrawRectangle(0, i * lines_y_step + (sin(ofSignedNoise(i, current_time)) * 10), size_x, size_y);
            }
        }
        // draw rects on second panel
        if (num_rects_h_1 > 0){
            // increase saturation
            saturation *= 2;
            // increase vertical size of the rectangles
            max_y_rect_size *= 2;
            ofTranslate(fbo->getWidth()/4, 0, 0);
            for (int i = num_rects_h_2; i > 0; i--){
                // size
                size_x = fbo->getWidth()/4;
                size_y = ofMap(ofSignedNoise(i, current_time*3), -1, 1, 2, max_y_rect_size);
                // colors
                hue = ofMap(i * lines_y_step, 0, fbo->getHeight(), 0, 255);
                color.setHsb(hue, saturation, darkness, 255);
                ofSetColor(color);
                ofDrawRectangle(0, i * lines_y_step + (sin(ofSignedNoise(i+2, current_time)) * 20), size_x, size_y);
            }
        }
        // draw rects on third panel
        if (num_rects_h_2 > 0){
            // increase saturation
            saturation *= 2;
            // increase vertical size of the rectangles
            max_y_rect_size *= 2;
            ofTranslate(fbo->getWidth()/4, 0, 0);
            for (int i = 0; i < num_rects_h_3; i++){
                // size
                size_x = fbo->getWidth()/4;
                size_y = ofMap(ofSignedNoise(i, current_time*3), -1, 1, 2, max_y_rect_size);
                // colors
                hue = ofMap(i * lines_y_step, 0, fbo->getHeight(), 0, 255);
                color.setHsb(hue, saturation, darkness, 255);
                ofSetColor(color);
                ofDrawRectangle(0, i * lines_y_step + (sin(ofSignedNoise(i-1, current_time)) * 30), size_x, size_y);
            }
        }
        // draw rects on fourth panel
        if (num_rects_h_3 > 0){
            // increase saturation
            saturation *= 2;
            // increase vertical size of the rectangles
            max_y_rect_size *= 2;
            ofTranslate(fbo->getWidth()/4, 0, 0);
            for (int i = num_rects_h_4; i > 0; i--){
                // size
                size_x = fbo->getWidth()/4;
                size_y = ofMap(ofSignedNoise(i, current_time*3), -1, 1, 2, max_y_rect_size);
                // colors
                hue = ofMap(i * lines_y_step, 0, fbo->getHeight(), 0, 255);
                color.setHsb(hue, saturation, darkness, 255);
                ofSetColor(color);
                ofDrawRectangle(0, i * lines_y_step + (sin(ofSignedNoise(i-1, current_time)) * 40), size_x, size_y);
            }
        }
        ofPopMatrix();
        ofDisableBlendMode();

        // when we reached the end, start the next animation
        if (num_rects_h_4 > 10 && !coloured_quads_started){
            coloured_quads_started = true;
            coloured_quads_start_time = current_time;
        }
    }

    ofEnableAlphaBlending();

    // fill canvas with quads of hue arranged colours
    if (coloured_quads_started){
        
        float duration = 1.0f; // duration of the animation of a single quad
        float alpha_duration = 0.8f; // duration of the alpha fade in
        float sat_duration = 12; // duration of the saturation fade out
        float bri_duration = 24; // duration of the brightness fade out
        float x_time_offset_multiplier = 0.020f; // used to delay quads on different cols
        
        int i = 0; // keeps track of the current index we're in

        float saturation, brightness;

        for (int y = 0; y < fbo->getHeight(); y+=quad_size){
            for (int x = 0; x < fbo->getWidth(); x+=quad_size){
                
                float time_offset = (x * x_time_offset_multiplier);
                
                // animate color, scale, rotation
                float scale_animated = ofMap(current_time, coloured_quads_start_time + time_offset, coloured_quads_start_time + time_offset+ duration, 0.001f, 1.0f, true);
                float rotation_animated = ofMap(current_time, coloured_quads_start_time + time_offset, coloured_quads_start_time + time_offset + duration, 90, 0, true);
                saturation = ofMap(current_time, coloured_quads_start_time + time_offset, coloured_quads_start_time + time_offset + sat_duration, 255, 0, true);
                brightness = ofMap(current_time, coloured_quads_start_time + time_offset + sat_duration, coloured_quads_start_time + time_offset + bri_duration, 120, 0, true);

                // pick the color that we previously assigned
                ofColor current_color = final_quads_colors.at(i);
                // fade in animation using the alpha
                current_color.a = ofMap(current_time, coloured_quads_start_time + time_offset, coloured_quads_start_time + time_offset + alpha_duration, 0, 255, true);
                current_color.setSaturation(saturation);
                current_color.setBrightness(brightness);

                ofPushMatrix();
                    ofTranslate(x, y, 0);
                    
                    ofRotateY(rotation_animated);
                    ofRotateZ(rotation_animated);

                    ofSetColor(current_color);
                    ofDrawRectangle(0, 0, quad_size * scale_animated, quad_size * scale_animated);

                ofPopMatrix();

                if (!bg_started_fade){
                    if (y >= fbo->getHeight() - quad_size && x >= fbo->getWidth() - quad_size){
                        if (current_color.a == 255 && scale_animated == 1.0f && rotation_animated == 0.0f){
                            if (saturation == 0 ){
                                if (brightness == 0){
                                    bg_started_fade = true;
                                    black_bg_fade_in_time = current_time;
                                }
                            }
                        }
                    }
                }

                i++;
            }
        }
    }

    if (bg_started_fade){
   	app_should_quit = true;
    }

    // we don't need this anymore
    // start the final fade out to black
    // if (bg_started_fade){
    //     float duration = 5;
    //     float animated_alpha = ofMap(current_time, black_bg_fade_in_time, black_bg_fade_in_time + duration, 0, 255, true);
    //     ofColor target_color(0, 0, 0, animated_alpha);
    //     ofSetColor(target_color);
    //     ofDrawRectangle(0, 0, fbo->getWidth(), fbo->getHeight());
    // }

    ofDisableAlphaBlending();

    ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
// EVENTS
//--------------------------------------------------------------
void VVSource::onKeyPressed(ofKeyEventArgs & event){
    switch (event.key){
        case 'd': {
            show_calibration_grid = !show_calibration_grid;
        }
    }
}
