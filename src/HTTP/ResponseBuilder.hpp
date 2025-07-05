/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseBuilder.hpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: user <user@student.42.fr>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/26 12:33:22 by user              #+#    #+#             */
/*   Updated: 2025/07/05 15:29:00 by user             ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Request.hpp"
#include "Response.hpp"
#include <fstream>

class ResponseBuilder {
public:
	static Response buildFileResponse(const std::string& filePath);
	static Response buildErrorResponse(int statusCode, const std::string& message);
	
	static std::string getFileExtension(const std::string& filePath);
	static bool fileExists(const std::string& filePath);
	static std::string getContentType(const std::string& filePath);
	
	static Response buildGetResponse(const requestParser& request, const std::string& docRoot);
	static Response buildPostResponse(const requestParser& request, const std::string& docRoot);
	static Response buildDeleteResponse(const requestParser& request, const std::string& docRoot);
	
};
