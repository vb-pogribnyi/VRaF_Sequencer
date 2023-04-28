#pragma once
#include <vector>
#include <string>
#include "glm.hpp"
#include "../imgui/imgui.h"

// Vector Recording and Filtering namespace
namespace VRaF {
	struct Event {
		mutable int time;  // Start time, to be precise
		mutable int duration;
		// pair<float, float> is Time, Value
		// In keyframes, time is a float from 0 to 1; in order to ease scaling
		std::vector<std::pair<float, float>> keyframes;
		void update(int frame);
		void filter(bool is_backwards);
		void filter();
		void clear();
		float* target = 0;
	};

	struct Recording {
		float* target;
		// As contrary to Event keyframes, this array holds
		// frame index as the key, thus the key is of type int
		// pair<int, float> is Frame, Value
		std::vector<std::pair<int, float>> keyframes;
		void update(int frame);
	};

	struct Track {
		ImColor color = ImColor::HSV(0.5f, 1.0f, 1.0f);
		std::vector<Event> events;
		std::vector<Recording> recordings;
		std::string label;
		bool is_expanded = true;
	};

	struct SeqState {
		bool isPlaying;
		float startTime;
		float currTime;
		int frame;
		ImVec2 zoom;
		ImVec2 pan;
		int range[2];
	};

	struct Dimentions {
		ImVec2 X;
		ImVec2 A;
		ImVec2 B;
		ImVec2 C;
		ImVec2 windowSize;
		float titlebarHeight;
	};

	enum SectionType {
		SECTION_CROSS,
		SECTION_LISTER,
		SECTION_TIMELINE,
		SECTION_EDITOR,

		SECTION_COMMON
	};

	class Sequencer
	{
	public:
		Sequencer(int fps=30);
		void toggle();
		void draw();
		void update(float time);

		// The target must be contained in an event first.
		// If the event contains multiple targets, all of them will be recorded
		void record(float* target);
		void track(std::string label, float* value);
		void track(std::string label, glm::vec2* value);
		void track(std::string label, glm::vec3* value);
		void track(std::string label, glm::vec4* value);

	private:
		SeqState state;
		std::vector<Track> tracks;
		int fps;

		Dimentions dims;
		void stop_recording();
		void drawBackground(SectionType section);
		void drawTracks(SectionType section);
		void drawGrid(SectionType section);
		void drawIndicators();
		void updateEvents();
		void updateEvents(int frame);
		ImFont* icons;
		ImFont* labels;
	};
}
