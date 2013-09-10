<?php

/**
 * @file
 *  The implementation of the OAuth2.0 authentication scheme
 */

/**
 * Authenticates a call using OAuth2.0 to verify the request.
 *
 * @param $settings
  *  The settings for the authentication module.
 * @param $method
 *  The method that's being called
 * @param $args
 *  The arguments that are being used to call the method
 *
 * @return
 *  Returns nothing, or a error message if authentication fails
 */
function _services_oauth2_authenticate_call($settings, $method, $args) {
    $result = oauth2_get_server()->verifyAccessToken();
    return $result === TRUE ? NULL : $result;
}

function _services_oauth2_security_settings() {
  return array();
}

function _services_oauth2_controller_settings($settings, $controller, $endpoint, $class, $name) {
  return array();
}
