<?php

/**
 * @file
 * Drupal OAuth2 Implementation
 */

class DrupalOAuth2 extends OAuth2 {

  /**
   * Overrides OAuth2::__construct().
   */
  public function __construct($config = array()) {
    global $user;

    parent::__construct($config);

    // devel_execute_block_form also submit execute code in $_REQUEST['code'].
    if (!isset($_REQUEST['form_id']) || $_REQUEST['form_id'] !== 'devel_execute_block_form') {
      $result = NULL;

      // Hack to corresponding user if oauth_token provided.
      if (isset($_REQUEST['oauth_token'])) {
        $result = $this->getAccessToken($_REQUEST['oauth_token']);
      }
      // Hack to corresponding user if code provided.
      elseif (isset($_REQUEST['code'])) {
        $result = $this->getAuthCode($_REQUEST['code']);
      }
      // Hack to corresponding user if refresh_token provided.
      elseif (isset($_REQUEST['refresh_token'])) {
        $result = $this->getRefreshToken($_REQUEST['refresh_token']);
      }

      if ($result) {

        // Convert object to array
        $result = (array) $result;

        $this->loadUserCredentials($result['uid'], $result['name'], $result['sid']);
      }
    }
  }

  /**
   * Load the user based on uid and name, assign with corresponding session;
   * or create dummy user on-the-fly.
   *
   * @param $uid
   *   The user ID.
   * @param $name
   *   The user name.
   * @param $sid
   *   The target session ID to be set with.
   *
   * @return
   *   The user object after successful login.
   */
  protected function loadUserCredentials($uid, $name, $sid) {
    global $user;

    if (isset($uid) && isset($name) && !($account = user_load($uid))) {
      // We hack and create a dummy user placeholder if it is exist in
      // token tables but not yet a real user account, typically
      // happened when server running in proxy mode. Can't use
      // drupal_write_record() as uid is serial.
      $id = db_insert('users')
  ->fields(array(
        'uid' => $uid,
        'name' => $name,
        'created' => REQUEST_TIME,
        'access' => REQUEST_TIME,
        'login' => REQUEST_TIME,
        'status' => 1,
      ))
  ->execute();
      $account = user_load($uid);
    }
    $user = $account;

    // Some client/servers, like XMLRPC, do not handle cookies, so
    // imitate it to make sess_read() function try to look for user,
    // instead of just loading anonymous user :).
    $session_name = session_name();
    if (!isset($_COOKIE[$session_name])) {
      $_COOKIE[$session_name] = $sid;
    }

    session_id($sid);
    return $user;
  }

  /**
   * Verify authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be check with.
   * @param $required_scope
   *   The required scope within current request.
   * @param $uid
   *   The user ID.
   *
   * @return
   *   A list of all extended scopes besides authorized scopes of end-user.
   */
  public function verifyAuthorizedScopes($client_id, $required_scope, $uid) {
    $authorized_scope = $this->getAuthorizedScopes($client_id, $uid);
    $hidden_scope = $this->getHiddenScopes();

    if (!is_array($required_scope)) {
      $required_scope = explode(" ", $required_scope);
    }
    $required_scope = array_unique(array_merge($required_scope, $hidden_scope));

    $scopes = $this->checkExtendedPermissions($required_scope, $authorized_scope);

    $extended_permissions = array();
    foreach ($scopes as $scope) {
      $stmt = db_query("SELECT os.nid, os.scope_id, os.hidden, os.bypass FROM {node} AS n INNER JOIN {oauth2_scopes} AS os ON n.vid = os.vid WHERE os.scope_id = :os.scope_id AND n.status = :n.status", array(':os.scope_id' => $scope, ':n.status' => 1));

      foreach ($stmt as $result) {
        $extended_permissions[] = get_object_vars($result);
      }
    }

    return array_filter($extended_permissions);
  }

  /**
   * Get all hidden scopes that should be automatically included within blank
   * scope request.
   *
   * @return
   *   A list for all hidden scopes.
   */
  public function getHiddenScopes() {

    $scopes = array();

    $temp  = "SELECT os.scope_id FROM {node} AS n INNER JOIN {oauth2_scopes} AS os ON n.vid = os.vid ";
    $temp .= " WHERE os.hidden = 1";
    $temp .= " AND n.status = 1";

    $stmt = db_query($temp);
    foreach ($stmt as $result) {
      $result = get_object_vars($result);
      $scopes[] = $result['scope_id'];
    }
    return $scopes;
  }

  /**
   * Get all authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be check with.
   * @param $uid
   *   The user ID.
   *
   * @return
   *   A list for all authorized scopes for this end-user.
   */
  public function getAuthorizedScopes($client_id, $uid) {

    $scopes = array();

    $temp  = "SELECT scope_id FROM {oauth2_authorize} ";
    $temp .= " WHERE client_id = '" . $client_id . "'";
    $temp .= " AND uid = " . $uid;

    $stmt = db_query($temp);

    foreach ($stmt as $result) {
      $results = get_object_vars($result);
      $scopes[] = $result['scope_id'];
    }
    return $scopes;
  }

  /**
   * Set authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be authorize.
   * @param $scopes
   *   The scopes as a list of space-delimited strings.
   * @param $uid
   *   The user ID.
   */
  public function setAuthorizedScopes($client_id, $scopes, $uid) {
    $scope_ids = explode(" ", $scopes);
    foreach ($scope_ids as $scope_id) {
      $id = db_insert('oauth2_authorize')
  ->fields(array(
        'client_id' => $client_id,
        'uid' => $uid,
        'scope_id' => $scope_id,
      ))
  ->execute();
    }
  }

  /**
   * Unset authorized scopes for end-user.
   *
   * @param $client_id
   *   The client identifier to be unauthorize.
   * @param $scopes
   *   The scopes as a list of space-delimited strings.
   * @param $uid
   *   The user id.
   */
  public function unsetAuthorizedScopes($client_id, $scopes, $uid) {
    $scope_ids = explode(" ", $scopes);
    foreach ($scope_ids as $scope_id) {
      db_delete('oauth2_authorize')
  ->condition('client_id', $client_id)
  ->condition('uid', $uid)
  ->condition('scope_id', $scope_id)
  ->execute();
    }
  }

  /**
   * Check if any extended permissions besides authorized scopes.
   *
   * @param $required_scope
   *   The target required scope within current request.
   * @param $authorized_scope
   *   Scopes that already authorized by end-user.
   *
   * @return
   *   A list with all extended permissions.
   */
  private function checkExtendedPermissions($required_scope, $authorized_scope) {
    // The required scope should match or be a subset of the available scope
    if (!is_array($required_scope)) {
      $required_scope = explode(" ", $required_scope);
    }

    if (!is_array($authorized_scope)) {
      $authorized_scope = explode(" ", $authorized_scope);
    }

    return array_filter(array_diff($required_scope, $authorized_scope));
  }

  /**
   * Expires all OAuth2.0 related tokens based on sid.
   *
   * @param $sid
   *   Session ID to be expires with.
   *
   * @return
   *   TRUE if successful, and FALSE if it isn't.
   */
  public function expireSession($sid) {
    $oauth_sid = '';

    // Purge tokens based on oauth_token if provided.
    if (isset($_REQUEST['oauth_token']) && !empty($_REQUEST['oauth_token'])) {
      $result = $this->getAccessToken($_REQUEST['oauth_token']);
      if ($result) {
        $oauth_sid = $result['sid'];
      }
    }

    // Purge tokens if session found.
    if ($oauth_sid != '') {
      if (!isset($_GET['redirect_uri'])) {
        // We hack $_REQUEST['redirect_uri'] so handle it custom logout.

        $sql = "SELECT oc.redirect_uri
          FROM {oauth2_access_tokens} AS oat
            INNER JOIN {oauth2_clients} AS oc ON oat.client_id = oc.client_id
            INNER JOIN {node} AS n ON oc.vid = n.vid
            WHERE oat.sid = '" . $oauth_sid . "' AND n.status = 1";

        $temp = db_query($sql, array(), array());

        foreach ($temp as $result) {
          $result = get_object_vars($result);
        }

        $_GET['redirect_uri'] = $result['redirect_uri'];
      }

      // Expire all tokens base on this session_id

      db_delete('oauth2_access_tokens')
  ->condition('sid', $oauth_sid)
  ->execute();

      db_delete('oauth2_auth_codes')
  ->condition('sid', $oauth_sid)
  ->execute();

      db_delete('oauth2_refresh_tokens')
  ->condition('sid', $oauth_sid)
  ->execute();

      // Also manually destroy user session.

//      if (session_id($sid) != $sid) {
      if (session_id() != $sid) {
        session_start();
        session_id($sid);
      }

      return session_destroy();
    }

    return FALSE;
  }

  /**
   * Implements OAuth2::checkClientCredentials().
   */
  public function checkClientCredentials($client_id, $client_secret = NULL) {

    $query = db_select('node', 'n');
    $query->innerJoin('oauth2_clients', 'oac', 'n.vid = oac.vid');
    $query->fields('oac', array('client_secret'));
    $query->condition('oac.client_id', $client_id, '=');
    $query->condition('n.status', 1, '=');

    $secret = $query->execute()->fetchField();

    return ($client_secret !== NULL && $secret == $client_secret) ? TRUE : FALSE;
  }

  /**
   * Implements OAuth2::getRedirectUri().
   */
  public function getRedirectUri($client_id) {

    $query = db_select('node', 'n');
    $query->innerJoin('oauth2_clients', 'oc', 'n.vid = oc.vid');
    $query->fields('oc', array('redirect_uri'));
    $query->condition('oc.client_id', $client_id, '=');
    $query->condition('n.status', 1, '=');

    $redirect_uri = $query->execute()->fetchField();

    return $redirect_uri !== FALSE ? $redirect_uri : NULL;
  }

  /**
   * Implements OAuth2::getAccessToken().
   */
  public function getAccessToken($oauth_token) {

    $query = db_select('oauth2_access_tokens', 'oat');
//    $query->innerJoin('user', 'u', 'oat.uid = u.uid');
    $query->innerJoin('users', 'u', 'oat.uid = u.uid');
//    $query->fields('oac', array('client_id', 'sid', 'expires'));
    $query->fields('oat', array('client_id', 'sid', 'expires'));
    $query->fields('u', array('uid', 'name'));
    $query->condition('oat.oauth_token', $oauth_token, '=');

//
// Note: oauth2-php wants token as an array, not an object
//
//    return (object) $query->execute()->fetchAssoc();
    $return_value = (array) $query->execute()->fetchAssoc();

// Note: cannot simply return the fecthAssoc(), because this will not match
//       "===NULL" in OAuth2.inc:verifyAccessToken() when
//       the token is not found
//
    if (!$return_value['uid']) {
      $return_value = NULL;
    }

    return $return_value;
  }

  /**
   * Implements OAuth2::setAccessToken().
   */
  public function setAccessToken($oauth_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;

    return $id = db_insert('oauth2_access_tokens')
  ->fields(array(
      'oauth_token' => $oauth_token,
      'client_id' => $client_id,
      'expires' => $expires,
      'scope' => $scope,
      'uid' => $uid,
      'sid' => $sid,
    ))
  ->execute();
  }

  /**
   * Overrides OAuth2::getSupportedGrantTypes().
   */
  protected function getSupportedGrantTypes() {
    return array(
      OAUTH2_GRANT_TYPE_AUTH_CODE,
      OAUTH2_GRANT_TYPE_USER_CREDENTIALS,
      OAUTH2_GRANT_TYPE_REFRESH_TOKEN
    );
  }

  /**
   * Overrides OAuth2::getSupportedScopes().
   */
  public function getSupportedScopes() {

    $query = db_select('node', 'n');
    $query->innerJoin('oauth2_scopes', 'os', 'n.vid = os.vid');
    $query->fields('os', array('scope_id'));
    $query->condition('n.status', 1, '=');

    return $query->execute()->fetchAll();

  }

  /**
   * Overrides OAuth2::getAuthCode().
   */
  public function getAuthCode($code) {

    $query = db_select('oauth2_auth_codes', 'oac');

    $query->innerJoin('users', 'u', 'oac.uid = u.uid');

    $query->fields('oac', array('client_id', 'sid', 'redirect_uri', 'expires', 'scope'))
          ->fields('u', array('uid', 'name'));

    $query->condition('oac.code', $code, '=');

    $stmt = $query->execute();

    foreach ($stmt as $result) {
      $result = get_object_vars($result);
    }

    return $result;
  }

  /**
   * Overrides OAuth2::setAuthCode().
   */
  public function setAuthCode($code, $client_id, $redirect_uri, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;

    return $id = db_insert('oauth2_auth_codes')
  ->fields(array(
      'code' => $code,
      'client_id' => $client_id,
      'redirect_uri' => $redirect_uri,
      'expires' => $expires,
      'scope' => $scope,
      'uid' => $uid,
      'sid' => $sid,
    ))
  ->execute();
  }

  /**
   * Overrides OAuth2::checkUserCredentials().
   */
  public function checkUserCredentials($client_id, $username, $password) {
    // Clone from user_service_login().
    $account = user_authenticate($username, $password);

    $result = NULL;

    if ($account) {
      // Regenerate the session ID to prevent against session fixation attacks.
      drupal_session_regenerate();
      $array = array();
      user_module_invoke('login', $array, $account);

      $result = array(
        'uid' => $account,
        'name' => $username,
        'sid' => session_id(),
        'scope' => trim(implode(' ', $this->getAuthorizedScopes($client_id, $account))),
      );
    }

    return $result ? $result : FALSE;
  }

  /**
   * Overrides OAuth2::getRefreshToken().
   */
  public function getRefreshToken($refresh_token) {
    $stmt = db_query("SELECT ort.client_id, ort.sid, ort.expires, ort.scope, u.uid, u.name
      FROM {oauth2_refresh_tokens} AS ort
        INNER JOIN {users} AS u ON ort.uid = u.uid
      WHERE ort.refresh_token = :ort.refresh_token", array(':ort.refresh_token' => $refresh_token));

    foreach ($stmt as $result) {
      $result = get_object_vars($result);
    }

    return $result;
  }

  /**
   * Overrides OAuth2::setRefreshToken().
   */
  public function setRefreshToken($refresh_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;

    return $id = db_insert('oauth2_refresh_tokens')
  ->fields(array(
      'refresh_token' => $refresh_token,
      'client_id' => $client_id,
      'expires' => $expires,
      'scope' => $scope,
      'uid' => $uid,
      'sid' => $sid,
    ))
  ->execute();
  }

  /**
   * Overrides OAuth2::unsetRefreshToken().
   */
  public function unsetRefreshToken($refresh_token) {
    return db_delete('oauth2_refresh_tokens')
  ->condition('refresh_token', $refresh_token)
  ->execute();
  }
}