#include "VRaFSequencer.h"
#include <iostream>
#include <string>
#include <filesystem>
namespace fs = std::filesystem;

// It's private flag in ImGui ImGuiButtonFlags_AllowItemOverlap; but SetItemAllowOverlap() function alone doesn't work
#define IMGUI_ALLOW_OVERLAP (1 << 12)

inline ImVec4 operator*(const ImVec4& vec, const float mult) {
	return ImVec4{ vec.x * mult, vec.y * mult, vec.z * mult, vec.w };
}

inline ImVec2 operator+(const ImVec2& vec, const ImVec2 value) {
	return ImVec2{ vec.x + value.x, vec.y + value.y };
}

inline ImVec4 operator+(const ImVec4& vec, const ImVec4 value) {
	return ImVec4{ vec.x + value.x, vec.y + value.y, vec.z + value.z, vec.w + value.w };
}

inline ImVec2 operator-(const ImVec2& vec, const ImVec2 value) {
	return ImVec2{ vec.x - value.x, vec.y - value.y };
}

inline ImVec2 operator*(const ImVec2& vec, const float value) {
	return ImVec2{ vec.x * value, vec.y * value };
}

namespace VRaF {

	static struct Theme_ {
		float headerWidth = 180.0;
		float headerHeight = 40.0;
		float trackHeight{ 25.0f };
		float handleWidth = 10.0;
	} Theme;

	/**
	* Sequencer is divided into 4 panels
	*
	*          _________________________________________________
	*         |       |                                         |
	*   Cross |   X   |                  B                      | Timeline
	*         |_______|_________________________________________|
	*         |       |                                         |
	*         |       |                                         |
	*         |       |                                         |
	*  Lister |   A   |                  C                      | Editor
	*         |       |                                         |
	*         |       |                                         |
	*         |_______|_________________________________________|
	*
	*/
	void Sequencer::drawBackground(SectionType section) {
		auto* painter = ImGui::GetWindowDrawList();

		auto crossBackground = [&]() {
			painter->AddRectFilled(
				dims.X,
				dims.X + ImVec2{ Theme.headerWidth + 1,  Theme.headerHeight },
				ImGui::GetColorU32(ImGuiCol_WindowBg)
			);

			// Border
			painter->AddLine(dims.X + ImVec2{ Theme.headerWidth, 0.0f },
				dims.X + ImVec2{ Theme.headerWidth, Theme.headerHeight },
				ImGui::GetColorU32(ImGuiCol_Border),
				ImGui::GetStyle().PopupBorderSize);

			painter->AddLine(dims.X + ImVec2{ 0.0f, Theme.headerHeight },
				dims.X + ImVec2{ Theme.headerWidth + 1, Theme.headerHeight },
				ImGui::GetColorU32(ImGuiCol_Border),
				ImGui::GetStyle().PopupBorderSize);

			ImVec2 offset(10.0f, ImGui::GetScrollY());
			ImVec4 btn_color(0, 0, 0, 0);
			float btn_width = 20;
			ImGui::PushStyleColor(ImGuiCol_Button, btn_color);
			ImGui::PushFont(icons);


			ImGui::SetCursorPos(dims.X - ImGui::GetWindowPos() + offset);
			if (ImGui::Button("B", ImVec2(0, Theme.headerHeight))) {
				state.frame = state.range[0];
				updateEvents();
			}
			offset.x += btn_width;

			ImGui::SetCursorPos(dims.X - ImGui::GetWindowPos() + offset);
			if (ImGui::Button("b", ImVec2(0, Theme.headerHeight))) {
				state.frame -= 1;
				if (state.frame < state.range[0]) state.frame = state.range[0];
				updateEvents();
			}
			offset.x += btn_width;

			ImGui::SetCursorPos(dims.X - ImGui::GetWindowPos() + offset);
			if (state.isPlaying && ImGui::Button("p", ImVec2(0, Theme.headerHeight))) toggle();
			if (!state.isPlaying && ImGui::Button("P", ImVec2(0, Theme.headerHeight))) toggle();
			offset.x += btn_width;

			ImGui::SetCursorPos(dims.X - ImGui::GetWindowPos() + offset);
			if (ImGui::Button("S", ImVec2(0, Theme.headerHeight))) {
				state.isPlaying = false;
				state.frame = state.range[0];
				updateEvents();
			}
			offset.x += btn_width;

			ImGui::SetCursorPos(dims.X - ImGui::GetWindowPos() + offset);
			if (ImGui::Button("d", ImVec2(0, Theme.headerHeight))) {
				state.frame += 1;
				if (state.frame > state.range[1]) state.frame = state.range[1];
				updateEvents();
			}
			offset.x += btn_width;

			ImGui::SetCursorPos(dims.X - ImGui::GetWindowPos() + offset);
			if (ImGui::Button("D", ImVec2(0, Theme.headerHeight))) {
				state.frame = state.range[1];
				updateEvents();
			}
			offset.x += btn_width;


			ImGui::PopFont();
			ImGui::PopStyleColor();
		};
		auto listerBackground = [&]() {
			// Fill
			painter->AddRectFilled(
				dims.A,
				dims.A + ImVec2{ Theme.headerWidth, dims.windowSize.y },
				ImGui::GetColorU32(ImGuiCol_WindowBg)
			);

			// Border
			painter->AddLine(
				dims.A + ImVec2{ Theme.headerWidth, 0.0f },
				dims.A + ImVec2{ Theme.headerWidth, dims.windowSize.y },
				ImGui::GetColorU32(ImGuiCol_Border),
				ImGui::GetStyle().PopupBorderSize
			);
		};
		auto timelineBackground = [&]() {
			// Fill
			painter->AddRectFilled(
				dims.B,
				dims.B + ImVec2{ dims.windowSize.x, Theme.headerHeight },
				ImGui::GetColorU32(ImGuiCol_WindowBg)
			);

			// Border
			painter->AddLine(dims.B + ImVec2{ 0.0f, Theme.headerHeight },
				dims.B + ImVec2{ dims.windowSize.x, Theme.headerHeight },
				ImGui::GetColorU32(ImGuiCol_Border),
				ImGui::GetStyle().PopupBorderSize);
		};
		auto editorBackground = [&]() {
			painter->AddRectFilled(
				dims.C,
				dims.C + ImVec2(dims.windowSize.x, dims.windowSize.y),
				ImColor(ImGuiCol_ChildBg)
			);
		};

		if (section == SECTION_CROSS) crossBackground();
		else if (section == SECTION_EDITOR) editorBackground();
		else if (section == SECTION_LISTER) listerBackground();
		else if (section == SECTION_TIMELINE) timelineBackground();
	}

	void Sequencer::drawTracks(SectionType section) {
		auto* painter = ImGui::GetWindowDrawList();
		auto trackHeader = [&](Track& track, ImVec2& cursor, int track_id) {
			// Buttons in the lister
			ImGui::PushID(track_id * 1000 - 1);

			const ImVec2 pos{ dims.X.x + 30, cursor.y - ImGui::GetScrollY() + Theme.trackHeight / 2 - 8 };
			painter->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text, 1.0), track.label.c_str());
			ImGui::SetCursorPos(cursor - ImGui::GetWindowPos());

			const char* label = "F";
			float btn_width = ImGui::CalcTextSize(label, NULL, true).x + 12;
			float cursor_y = ImGui::GetCursorPosY();

			ImGui::SetCursorPos({ btn_width / 4, cursor_y });
			ImVec4 btn_color(0, 0, 0, 0);

			ImGui::PushStyleColor(ImGuiCol_Button, btn_color);
			ImGui::PushFont(icons);

			if (track.is_expanded && ImGui::Button("E", ImVec2(0, Theme.trackHeight))) 
				track.is_expanded = !track.is_expanded;
			if (!track.is_expanded && ImGui::Button("e", ImVec2(0, Theme.trackHeight))) 
				track.is_expanded = !track.is_expanded;
			ImGui::SetCursorPos({ Theme.headerWidth - btn_width, cursor_y });
			if (ImGui::Button("F", ImVec2(0, Theme.trackHeight))) {
				for (Event& e : track.events) {
					e.filter();
				}
			}
			ImGui::SetCursorPos({ Theme.headerWidth - btn_width * 2, cursor_y });
			if (ImGui::Button("R", ImVec2(0, Theme.trackHeight))) {
				for (Event& e : track.events) {
					record(e.target);
				}
			}
			ImGui::SetCursorPos({ Theme.headerWidth - btn_width * 3, cursor_y });
			if (ImGui::Button("C", ImVec2(0, Theme.trackHeight))) {
				for (Event& e : track.events) {
					e.clear();
				}
				track.recordings.clear();
			}

			ImGui::PopFont();
			ImGui::PopStyleColor();

			ImGui::PopID();

			cursor.y += Theme.trackHeight;
		};

		// Draw track header, a separator-like empty space
		// 
		// |__|__|__|__|__|__|__|__|__|__|__|__|__|__|
		// |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
		// |__|__|__|__|__|__|__|__|__|__|__|__|__|__|
		// |                                         |
		// |_________________________________________|
		// |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
		// |__|__|__|__|__|__|__|__|__|__|__|__|__|__|
		// |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
		//
		auto trackEditor = [&](const Track& track, ImVec2& cursor, int track_id) {
			const ImVec2 size{ dims.windowSize.x, Theme.trackHeight };

			float factor = sin(state.currTime * 3) * 0.5 + 0.5; 
			ImVec4 track_clr = ImGui::GetStyleColorVec4(ImGuiCol_Header);
			if (track.recordings.size() > 0) {
				track_clr = ImVec4(ImColor::HSV(0.0f, 1.00f, 0.600f)) * factor + track_clr * (1 - factor);
			}
			track_clr.w = 0.7;
			// Tint empty area with the track color
			painter->AddRectFilled(
				ImVec2{ dims.C.x, cursor.y - ImGui::GetScrollY() },
				ImVec2{ dims.C.x, cursor.y - ImGui::GetScrollY() } + size,
				ImColor(track_clr)
			);

			cursor.y += size.y;
		};

		/**
		 * @brief MIDI-like representation of on/off events
		 *
		 *   _______       _______________
		 *  |_______|     |_______________|
		 *       _______________
		 *      |_______________|
		 *                          _________________________
		 *                         |_________________________|
		 *
		 */
		auto eventEditor = [&](const Event& event, ImVec2& cursor, int track_id, int event_id) {
			float borderWidth = ImGui::GetStyle().PopupBorderSize;
			const ImVec2 pos{ 
				event.time * state.zoom.x + dims.C.x + state.pan.x + borderWidth,
				cursor.y - ImGui::GetScrollY() + borderWidth
			};
			const ImVec2 size { 
				event.duration * state.zoom.x - 2 * borderWidth, 
				Theme.trackHeight - 2 * borderWidth
			};
			cursor.y += size.y + 2 * borderWidth;
			ImVec2 head_tail_size
			{
				Theme.handleWidth,
				size.y + 2 * borderWidth
			};
			ImVec2 head_pos{ pos.x - borderWidth, pos.y - borderWidth };
			ImVec2 tail_pos{ pos.x + borderWidth + size.x - head_tail_size.x , pos.y - borderWidth };
			ImVec2 body_pos{ pos.x + head_tail_size.x, pos.y };
			ImVec2 body_size{ size.x - 2 * head_tail_size.x, size.y };


			if (event.duration <= 0) {
				// Reserve space
				ImGui::SetCursorPos(pos - ImGui::GetWindowPos() - ImVec2(0, -ImGui::GetScrollY()));
				ImGui::InvisibleButton("##stub", ImVec2{ 1, head_tail_size.y });
				return;
			}


			// Transitions
			float target_height{ 0.0f };

			/** Implement crop interaction and visualisation
				 ____________________________________
				|      |                      |      |
				| head |         body         | tail |
				|______|______________________|______|

			*/
			static float initial_time{ 0 };
			static float initial_length{ 0 };
			ImGui::PushID(track_id * 1000 + event_id);

			ImGui::SetCursorPos(head_pos - ImGui::GetWindowPos() - ImVec2(0, -ImGui::GetScrollY()));
			//ImGui::SetItemAllowOverlap();
			ImGui::InvisibleButton("##event_head", head_tail_size, IMGUI_ALLOW_OVERLAP);

			bool head_hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();

			if (ImGui::IsItemActivated()) {
				initial_time = event.time;
				initial_length = event.duration;
			}

			if (ImGui::IsItemActive()) {
				float delta = ImGui::GetMouseDragDelta().x;
				event.time = initial_time + round(delta / state.zoom.x);
				event.duration = initial_length - round(delta / state.zoom.x);
				if (event.duration < 1) {
					event.duration = 1;
					event.time = initial_time + initial_length - 1;
				}
				if (event.time < state.range[0]) {
					event.time = state.range[0];
					event.duration = initial_length + (initial_time - event.time);
				}
			}

			ImGui::SetCursorPos(tail_pos - ImGui::GetWindowPos() - ImVec2(0, -ImGui::GetScrollY()));
			//ImGui::SetItemAllowOverlap();
			ImGui::InvisibleButton("##event_tail", head_tail_size, IMGUI_ALLOW_OVERLAP);

			bool tail_hovered = ImGui::IsItemHovered() || ImGui::IsItemActive();

			if (ImGui::IsItemActivated()) {
				initial_length = event.duration;
			}
			if (ImGui::IsItemActive()) {
				float delta = ImGui::GetMouseDragDelta().x;
				event.duration = initial_length + round(delta / state.zoom.x);
				if (event.time + event.duration > state.range[1]) event.duration = state.range[1] - event.time;
				if (event.duration < 1) event.duration = 1;
			}

			ImGui::SetCursorPos(body_pos - ImGui::GetWindowPos() - ImVec2(0, -ImGui::GetScrollY()));
			ImGui::SetItemAllowOverlap();
			ImGui::InvisibleButton("##event", body_size, IMGUI_ALLOW_OVERLAP);
			ImGui::SetItemAllowOverlap();
			if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			}

			if (ImGui::IsItemActivated()) {
				initial_time = event.time;
			}
			if (ImGui::IsItemActive()) {
				float delta = ImGui::GetMouseDragDelta().x;
				event.time = initial_time + round(delta / state.zoom.x);
				if (event.time < state.range[0]) event.time = state.range[0];
				if (event.time + event.duration > state.range[1]) event.time = state.range[1] - event.duration;
			}

			ImGui::PopID();

			float halfBorder = borderWidth / 2;
			painter->AddRectFilled(
				pos + ImVec2(-borderWidth, -borderWidth),
				pos + ImVec2(borderWidth, borderWidth) + size,
				ImGui::GetColorU32(ImGuiCol_Button, 1.0)
			);
			painter->AddLine(
				pos + ImVec2(-borderWidth, -halfBorder),
				pos + ImVec2(borderWidth, -halfBorder) + ImVec2(size.x, 0),
				ImGui::GetColorU32(ImGuiCol_Border, 1.0), borderWidth);
			painter->AddLine(
				pos + ImVec2(-borderWidth, halfBorder + size.y),
				pos + ImVec2(borderWidth, halfBorder + size.y) + ImVec2(size.x, 0),
				ImGui::GetColorU32(ImGuiCol_Border, 1.0), borderWidth);
			painter->AddLine(
				pos + ImVec2(-halfBorder, 0),
				pos + ImVec2(-halfBorder, 0 + size.y),
				ImGui::GetColorU32(ImGuiCol_Border, 1.0), borderWidth);
			painter->AddLine(
				pos + ImVec2(halfBorder, 0) + ImVec2(size.x, 0),
				pos + ImVec2(halfBorder, 0 + size.y) + ImVec2(size.x, 0),
				ImGui::GetColorU32(ImGuiCol_Border, 1.0), borderWidth);
			// Keyframes curve
			if (event.keyframes.size() > 0) {
				float keymax = 0, keymin = 9999;
				for (const std::pair<float, float>& key : event.keyframes) {
					if (key.second > keymax) keymax = key.second;
					if (key.second < keymin) keymin = key.second;
				}
				float scale = size.y / (keymax - keymin);
				ImVec2 last_point(0, size.y - (event.keyframes[0].second - keymin) * scale);
				for (const std::pair<float, float>& key : event.keyframes) {
					ImVec2 curr_point(event.duration * key.first * state.zoom.x, size.y - (key.second - keymin) * scale);
					painter->AddLine(
						pos - ImVec2(borderWidth, 0) + last_point, 
						pos - ImVec2(borderWidth, 0) + curr_point,
						ImGui::GetColorU32(ImGuiCol_ButtonHovered, 1.0));
					last_point = curr_point;
				}
			}

			if (head_hovered)
			{
				painter->AddRectFilled(
					head_pos,
					head_pos + head_tail_size,
					ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive)
				);
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			if (tail_hovered) {
				painter->AddRectFilled(
					tail_pos,
					tail_pos + head_tail_size,
					ImGui::GetColorU32(ImGuiCol_ScrollbarGrabActive)
				);
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
		};

		ImVec2 cursor(dims.C.x + state.pan.x, dims.C.y);
		int event_id = 0, track_id = 0;
		if (section == SECTION_LISTER) {
			cursor = { dims.X.x, dims.C.y };
			event_id = track_id = 0;
			for (Track& track : tracks) {
				trackHeader(track, cursor, track_id);
				if (track.is_expanded) {
					for (Event& e : track.events) {
						cursor.y += Theme.trackHeight;
						event_id++;
					}
				}
				track_id++;
			}
		}
		else if (section == SECTION_EDITOR) {
			ImVec2 cursor(dims.C.x + state.pan.x, dims.C.y);
			int event_id = 0, track_id = 0;
			for (Track& track : tracks) {
				trackEditor(track, cursor, track_id);
				if (!track.is_expanded) continue;
				for (Event& e : track.events) {
					eventEditor(e, cursor, track_id, event_id);
					event_id++;
				}
				track_id++;
			}
		}
	}

	void Sequencer::drawGrid(SectionType section) {
		/**
		* Vertical grid lines
		*
		*  |    |    |    |    |    |    |    |    |    |    |    |
		*   ______________________________________________________
		*  |    |    |    |    |    |    |    |    |    |    |    |
		*  |    |    |    |    |    |    |    |    |    |    |    |
		*  |    |    |    |    |    |    |    |    |    |    |    |
		*  |    |    |    |    |    |    |    |    |    |    |    |
		*  |    |    |    |    |    |    |    |    |    |    |    |
		*  |____|____|____|____|____|____|____|____|____|____|____|
		*
		*/
		auto* painter = ImGui::GetWindowDrawList();

		int n_substeps = 5;
		float interstep_dist = state.zoom.x;
		float min_interstep = 50; // At least 20 pixels must be between 2 time signs
		int multiplier = ceil(min_interstep / interstep_dist / n_substeps);
		// Step must be multiple of n_substeps
		int step = n_substeps * multiplier;

		int min_time = (int)(-state.pan.x / state.zoom.x / step - 1) * step;
		int max_time = (int)((dims.windowSize.x - state.pan.x) / state.zoom.x / step + 1) * step;
		auto timelineGrid = [&]() {
			for (int time = min_time; time < max_time; time += step) {
				float xMin = time * state.zoom.x;
				float xMax = 0;
				float yMin = 0;
				float yMax = Theme.headerHeight - 1;
				xMin += dims.B.x + state.pan.x;
				xMax += dims.B.x + state.pan.x;
				yMin += dims.B.y;
				yMax += dims.B.y;

				painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), ImGui::GetColorU32(ImGuiCol_TableBorderStrong, 1.0));
				painter->AddText(
					ImGui::GetFont(),
					ImGui::GetFontSize() * 0.85f,
					ImVec2{ xMin + 5.0f, yMin },
					ImGui::GetColorU32(ImGuiCol_Text, 1.0),
					std::to_string(time).c_str()
				);

				for (int i = 0; i < n_substeps; i++) {
					painter->AddLine(
						ImVec2(xMin + (i + 1) * state.zoom.x * step / n_substeps, yMin + Theme.headerHeight * 0.3),
						ImVec2(xMin + (i + 1) * state.zoom.x * step / n_substeps, yMax),
						ImGui::GetColorU32(ImGuiCol_TableBorderLight, 1.0)
					);
				}
			}

			ImGui::SetCursorPos(dims.B - ImGui::GetWindowPos() - ImVec2(0, -ImGui::GetScrollY()));
			ImGui::InvisibleButton("##time_fast_select", ImVec2{ dims.windowSize.x, Theme.headerHeight }, IMGUI_ALLOW_OVERLAP);
			ImGui::SetItemAllowOverlap();
			if (ImGui::IsItemActive()) {
				ImGuiIO& io = ImGui::GetIO();
				float coord_curr = io.MousePos.x - dims.B.x - state.pan.x;
				state.frame = (int)round(coord_curr / state.zoom.x);
				updateEvents();
				state.isPlaying = false;
			}
		};

		auto editorGrid = [&]() {
			for (int time = min_time; time < max_time; time += step) {
				float xMin = time * state.zoom.x;
				float xMax = 0.0f;
				float yMin = 0.0f;
				float yMax = dims.windowSize.y;

				xMin += dims.C.x + state.pan.x;
				xMax += dims.C.x + state.pan.x;
				yMin += dims.C.y;
				yMax += dims.C.y;

				painter->AddLine(ImVec2(xMin, yMin), ImVec2(xMin, yMax), ImGui::GetColorU32(ImGuiCol_TableBorderStrong, 1.0));

				for (int z = 0; z < n_substeps - 1; z++) {
					const auto innerSpacing = state.zoom.x * step / n_substeps;
					auto subline = innerSpacing * (z + 1);
					painter->AddLine(ImVec2(xMin + subline, yMin), ImVec2(xMin + subline, yMax), ImGui::GetColorU32(ImGuiCol_TableBorderLight, 1.0));
				}
			}
		};

		if (section == SECTION_EDITOR) editorGrid();
		else if (section == SECTION_TIMELINE) timelineGrid();
	}

	void Sequencer::drawIndicators() {
		/**
		 * @brief Draw time indicator
		 *         __
		 *        |  |
		 *  _______\/___________________________________________________
		 *         |
		 *         |
		 *         |
		 *         |
		 *         |
		 *
		 */
		auto* painter = ImGui::GetWindowDrawList();
		int indicator_count = 0;


		auto timeIndicator = [&](int& time, ImVec4 cursor_color, ImVec4 line_color) {
			float x = time * state.zoom.x + dims.B.x + state.pan.x;
			float yMin = Theme.headerHeight + dims.B.y;
			float yMax = dims.windowSize.y + dims.B.y;
			painter->AddLine(
				ImVec2(x, yMin), ImVec2(x, yMax),
				ImColor(line_color), 2.0f
			);
			/**
			 *  Cursor
			 *    __
			 *   |  |
			 *    \/
			 *
			 */
			ImVec2 size{ 10.0f, 20.0f };
			auto topPos = ImVec2{ x, yMin };

			ImGui::PushID(indicator_count);
			ImGui::SetCursorPos(topPos - ImVec2{ size.x, size.y } - ImGui::GetWindowPos() - ImVec2(0, -ImGui::GetScrollY()));
			ImGui::SetItemAllowOverlap();
			ImGui::InvisibleButton("##indicator", size * 2.0f, IMGUI_ALLOW_OVERLAP);
			ImGui::PopID();

			static int initial_time{ 0 };
			if (ImGui::IsItemActivated()) {
				initial_time = time;
			}

			if (ImGui::IsItemActive()) {
				time = initial_time + static_cast<int>(ImGui::GetMouseDragDelta().x / state.zoom.x);
				if (time < state.range[0]) time = state.range[0];
				if (time > state.range[1]) time = state.range[1];
				updateEvents();
			}

			ImVec4 color = cursor_color;
			if (ImGui::IsItemHovered()) {
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
				color = color * 1.2f;
			}

			ImVec2 points[5] = {
				topPos,
				topPos - ImVec2{ -size.x, size.y / 2.0f },
				topPos - ImVec2{ -size.x, size.y },
				topPos - ImVec2{ size.x, size.y },
				topPos - ImVec2{ size.x, size.y / 2.0f }
			};

			painter->AddConvexPolyFilled(points, 5, ImColor(color));
			painter->AddPolyline(points, 5, ImColor(color * 1.25f), true, 1.0f);

			indicator_count++;
		};
		auto range = [&]() {
			ImVec2 cursor{ dims.C.x, dims.C.y };
			ImVec2 range_cursor_start{ state.range[0] * state.zoom.x + state.pan.x, Theme.headerHeight };
			ImVec2 range_cursor_end{ state.range[1] * state.zoom.x + state.pan.x, Theme.headerHeight };

			painter->AddRectFilled(
				cursor + ImVec2{ 0.0f, 0.0f },
				cursor + ImVec2{ range_cursor_start.x, dims.windowSize.y },
				ImColor(0.0f, 0.0f, 0.0f, 0.3f)
			);

			painter->AddRectFilled(
				cursor + ImVec2{ range_cursor_end.x, 0.0f },
				cursor + ImVec2{ dims.windowSize.x, dims.windowSize.y },
				ImColor(0.0f, 0.0f, 0.0f, 0.3f)
			);
		};

		timeIndicator(state.range[0], ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab), ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabActive));
		timeIndicator(state.range[1], ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrab), ImGui::GetStyleColorVec4(ImGuiCol_ScrollbarGrabActive));
		timeIndicator(state.frame, ImGui::GetStyleColorVec4(ImGuiCol_SliderGrab), ImGui::GetStyleColorVec4(ImGuiCol_SliderGrabActive));
		range();

	}

	Sequencer::Sequencer(int fps) : fps(fps)
	{
		fs::path p = fs::current_path();
		std::cout << p << std::endl;
		if (!fs::exists("icons.ttf") || !fs::exists("default.ttf")) {
			std::cout << "Could not find icons.ttf or default.ttf fonts" << std::endl;
			labels = ImGui::GetFont();
			icons = ImGui::GetFont();
		} else {
			ImGuiIO& io = ImGui::GetIO();
			// For some reason, any font loaded first substitutes the default one
			labels = io.Fonts->AddFontFromFileTTF("default.ttf", 16);
			icons = io.Fonts->AddFontFromFileTTF("icons.ttf", 18);
		}

		dims.titlebarHeight = 18.0f;
		state = {
			.isPlaying = false,
			.startTime = 0,
			.currTime = 0,
			.frame = 0,
			.zoom = { 10.0, 1 },
			.pan = { 0, 0 },
			.range = {1, 50}
		};
		if (state.frame < state.range[0]) state.frame = state.range[0];
		if (state.frame > state.range[1]) state.frame = state.range[1];
	}

	void Sequencer::draw()
	{
		dims.windowSize = ImGui::GetWindowSize();
		const ImVec2 windowPos = ImGui::GetWindowPos() + ImVec2{ 0.0f, dims.titlebarHeight };

		dims.X = windowPos;
		dims.A = windowPos + ImVec2{ 0.0f, Theme.headerHeight };
		dims.B = windowPos + ImVec2{ Theme.headerWidth, 0.0f };
		dims.C = windowPos + ImVec2{ Theme.headerWidth, Theme.headerHeight };


		drawBackground(SECTION_COMMON);
		drawTracks(SECTION_EDITOR);
		drawBackground(SECTION_TIMELINE);
		drawGrid(SECTION_TIMELINE);
		drawGrid(SECTION_EDITOR);

		drawIndicators();
		
		drawBackground(SECTION_LISTER);
		drawTracks(SECTION_LISTER);
		drawBackground(SECTION_CROSS);

		if (ImGui::IsKeyPressed(ImGuiKey_Space)) toggle();

		// Navigation
		ImGuiIO& io = ImGui::GetIO();
		if (io.WantCaptureMouse && ImGui::IsWindowHovered()) {
			if (io.MouseWheel != 0 && io.MousePos.x > dims.C.x) {
				float zoom_upd = state.zoom.x + io.MouseWheel;
				if (zoom_upd < 0.1) zoom_upd = 0.1;
				float coord_curr = io.MousePos.x - dims.C.x - state.pan.x;
				float coord_new = coord_curr / state.zoom.x * zoom_upd;

				state.zoom.x = zoom_upd;
				state.pan.x -= coord_new - coord_curr;
			}
			if (ImGui::IsMouseDown(2) && io.MouseDelta.x != 0) state.pan.x += io.MouseDelta.x;

			if (ImGui::IsMouseDown(2) && io.MouseDelta.y != 0) {
				state.pan.y += io.MouseDelta.y;
				if (state.pan.y < -ImGui::GetScrollMaxY()) state.pan.y = -ImGui::GetScrollMaxY();
				if (state.pan.y > 0) state.pan.y = 0;
				ImGui::SetScrollY(-state.pan.y);
			}
		}
	}

	void Sequencer::update(float time)
	{
		state.currTime = time;
		if (state.isPlaying) {
			int frame = state.frame;
			if (state.frame < state.range[0]) state.frame = state.range[0];
			if (state.frame > state.range[1]) {
				state.startTime = state.currTime;
				stop_recording();
			}
			state.frame = (state.currTime - state.startTime) * fps + state.range[0];
			if (state.frame != frame) {
				updateEvents(frame);
			}
		}
	}

	void Sequencer::updateEvents() {
		updateEvents(state.frame);
	}

	void Sequencer::updateEvents(int frame) {
		for (Track& track : tracks) {
			for (Event& e : track.events) {
				if (e.time <= frame && e.time + e.duration >= frame) {
					e.update(frame);
				}
			}
			for (Recording& r : track.recordings) {
				r.update(frame);
			}
		}
	}

	void Sequencer::record(float* target)
	{
		// Check if the target is being recorded
		Track* tgt_track = 0;
		for (Track& t : tracks) {
			for (Recording& r : t.recordings) {
				if (r.target == target) return;
			}
			for (Event& e : t.events) {
				if (e.target == target) {
					t.recordings.push_back({
						.target = target
						});
				}
			}
		}
	}

	void Sequencer::stop_recording()
	{
		// Transform all the recordings into events
		for (Track& t : tracks) {
			for (Recording& r : t.recordings) {
				int time = r.keyframes[0].first;
				int duration = r.keyframes[r.keyframes.size() - 1].first - time;
				std::vector<std::pair<float, float>> event_frames;
				for (Event& e : t.events) {
					if (e.target == r.target) {
						// TODO: Overwrite only the section captured by the recording
						e.keyframes.clear();
						e.time = time;
						e.duration = duration;
						for (std::pair<int, float> frame : r.keyframes) {
							e.keyframes.push_back({ (float)(frame.first - time) / duration, frame.second });
						}
					}
				}

			}
			t.recordings.clear();
		}
	}

	void Sequencer::track(std::string label, glm::vec2* value)
	{
		tracks.push_back({
			.events = { { 0, 0, {}, &(value->x) }, { 0, 0, {}, &(value->y) }},
			.label = label }
		);
	}

	void Sequencer::track(std::string label, glm::vec3* value)
	{
		tracks.push_back({
			.events = { { 0, 0, {}, &(value->x) }, { 0, 0, {}, &(value->y) }, { 0, 0, {}, &(value->z) }},
			.label = label }
		);
	}

	void Sequencer::track(std::string label, glm::vec4* value)
	{
		tracks.push_back({
			.events = { { 0, 0, {}, &(value->x) }, { 0, 0, {}, &(value->y) }, { 0, 0, {}, &(value->z) }, { 0, 0, {}, &(value->w) }},
			.label = label }
		);
	}

	void Sequencer::track(std::string label, float* value)
	{
		tracks.push_back({
			.events = { { 0, 0, {}, value }},
			.label = label }
		);
	}

	void Sequencer::toggle()
	{
		if (state.isPlaying) {
			state.isPlaying = false;
			stop_recording();
		}
		else {
			state.isPlaying = true;
			state.startTime = state.currTime - (float)(state.frame - state.range[0]) / fps;
		}
	}

	void Event::update(int frame)
	{
		float frameNorm = (float)(frame - time) / duration;
		float value = 0;
		for (auto& [t, v] : keyframes) {
			if (t <= frameNorm) value = v;
		}
		*target = value;
	}

	void Event::filter(bool is_backwards)
	{
		// First-order Butterworth filter coefficients
		// These coefficients obtained in python via lines:
		// ...
		// from scipy import signal
		// b, a = signal.butter(1, 0.4)

		float b[] = { 0.42080778, 0.42080778 };
		float a[] = { 1., -0.15838444 };
		if (is_backwards) std::reverse(keyframes.begin(), keyframes.end());

		float last_x = keyframes[0].second;
		float last_y = last_x;
		for (std::pair<float, float>& key : keyframes) {
			float y = b[0] * key.second + b[1] * last_x - a[1] * last_y;
			last_x = key.second;
			last_y = y;
			key.second = y;
		}
		if (is_backwards) std::reverse(keyframes.begin(), keyframes.end());
	}

	void Event::filter()
	{
		if (keyframes.size() == 0) return;
		filter(false);
		filter(true);
	}

	void Event::clear()
	{
		keyframes.clear();
		time = 0;
		duration = 0;
	}
	void Recording::update(int frame)
	{
		keyframes.push_back({ frame, *target });
	}

	SeqIterator Sequencer::begin() {
		state.frame = state.range[0];
		updateEvents();
		state.isPlaying = false;

		return SeqIterator(state.frame, this);
	}

	SeqIterator Sequencer::end() {
		state.frame = state.range[1] + 1;
		
		return SeqIterator(state.frame, this);
	}

	SeqIterator::SeqIterator(int frame, Sequencer* target) : frame(frame), target(target)
	{
	}
	SeqIterator SeqIterator::operator++()
	{
		frame++;
		target->state.frame = frame;
		target->updateEvents();
		return *this;
	}
	SeqIterator SeqIterator::operator++(int)
	{
		SeqIterator result = *this;
		++(*this);
		return result;
	}
}
