<?php

/**
 * @file
 *  Link general user functionalities to services module.
 */

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_verify_authorized_scopes($client_id, $required_scope, $uid) {
  return oauth2_get_server()->verifyAuthorizedScopes($client_id, $required_scope, $uid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_hidden_scopes() {
  return oauth2_get_server()->getHiddenScopes();
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_authorized_scopes($client_id, $required_scope, $uid) {
  return oauth2_get_server()->getAuthorizedScopes($client_id, $required_scope, $uid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_set_authorized_scopes($client_id, $scopes, $uid) {
  return oauth2_get_server()->setAuthorizedScopes($client_id, $scopes, $uid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_unset_authorized_scopes($client_id, $scopes, $uid) {
  return oauth2_get_server()->unsetAuthorizedScopes($client_id, $scopes, $uid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_expire_session($sid) {
  return oauth2_get_server()->expireSession($sid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_check_client_credentials($client_id, $client_secret = NULL) {
  return oauth2_get_server()->checkClientCredentials($client_id, $client_secret);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_redirect_uri($client_id) {
  return oauth2_get_server()->getRedirectUri($client_id);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_access_token($oauth_token) {
  return oauth2_get_server()->getAccessToken($oauth_token);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_set_access_token($oauth_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
  return oauth2_get_server()->setAccessToken($oauth_token, $client_id, $expires, $scope, $uid, $sid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_supported_scopes() {
  return oauth2_get_server()->getSupportedScopes();
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_auth_code($code) {
  return oauth2_get_server()->getAuthCode($code);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_set_auth_code($code, $client_id, $redirect_uri, $expires, $scope = '', $uid = 0, $sid = '') {
  return oauth2_get_server()->setAuthCode($code, $client_id, $redirect_uri, $expires, $scope, $uid, $sid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_check_user_credentials($client_id, $username, $password) {
  return oauth2_get_server()->checkUserCredentials($client_id, $username, $password);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_get_refresh_token($refresh_token) {
  return oauth2_get_server()->getRefreshToken($refresh_token);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_set_refresh_token($refresh_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
  return oauth2_get_server()->setRefreshToken($refresh_token, $client_id, $expires, $scope, $uid, $sid);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function oauth2_resource_unset_refresh_token($refresh_token) {
  return oauth2_get_server()->unsetRefreshToken($refresh_token);
}