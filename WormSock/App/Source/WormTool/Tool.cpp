/*
 * Tool.cpp
 *
 *  Created on: Oct 14, 2015
 *      Author: Eclipse C++
 */

#include <dirent.h>
#include <stddef.h>
#include <cstring>
#include <functional>
#include <string>



namespace App {
	namespace WormTool {
		bool match(std::string::const_iterator patb, std::string::const_iterator pate, const std::string& input) {
			std::string token;
			std::string::const_iterator wcard;
			bool jump = false;
			size_t submatch = 0;
			int matchTimes = 0;
			while (patb != pate) {
				jump = false;
				for (wcard = patb; wcard != pate; ++wcard)
					if ('*' == *wcard) break;

				token = std::string(patb, wcard);

				for (patb = wcard; patb != pate; ++patb) {
					if ('*' != *patb)
						break;
					else
						jump = true;
				}

				if (!token.empty()) {
					submatch = input.find(token, submatch);
					if (matchTimes == 0) {
						if (submatch != 0) return false;
					} else if (patb == pate) {
						if (jump) {
							if (std::string::npos == input.find(token, submatch)) return false;
						} else {
							if (input.substr(input.size() - token.size()) != token) return false;
						}
					}

					if (std::string::npos == submatch) {
						return false;
					}
				}
				matchTimes++;
			}
			if (matchTimes == 0) {
				if (input == "") return true;
				return false;
			}
			return true;
		}

		bool match(const std::string& pattern, const std::string& input) {
			return match(pattern.begin(), pattern.end(), input);
		}

		int traverseFolder(const char *path, std::function<bool(struct dirent& ent)> func) {
			int count = 0;
			struct dirent* ent = NULL;
			DIR *pDir;

			if ((pDir = opendir(path)) == NULL) {
				return -1;
			}

			while ((ent = readdir(pDir)) != NULL) {
				if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
				if (!func(*ent)) {
					closedir(pDir);
					return -2;
					break;
				}
				count++;
			}
			closedir(pDir);

			return count;
		}
	}
} /* namespace App */
