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

    $oauth2_mode = variable_get('oauth2_provider_mode');
    //only for standalone mode and no need to load if already logged in
    if ($oauth2_mode == 'DrupalOAuth2' && !isset($user->uid)) {

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
            $this->loadUserCredentials($result['uid'], $result['name'], $result['sid']);
          }
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
      db_query("INSERT INTO {users} (uid, name, created, access, login, status) VALUES (:uid, :name, :created, :access, :login, 1)", array(':uid' => $uid, ':name' => $name, ':created' => time(), ':access' => time(), ':login' => time()));
      $account = user_load($uid);
    }
    $user = $account;

    //login the user
    //copy from user_login_finalize without calling user_module_invoke('login', $edit, $user);
    watchdog('user', 'Session opened for %name.', array('%name' => $user->name));
    // Update the user table timestamp noting user has logged in.
    // This is also used to invalidate one-time login links.
    $user->login = REQUEST_TIME;
    db_update('users')
    ->fields(array('login' => $user->login))
    ->condition('uid', $user->uid)
    ->execute();

    // Regenerate the session ID to prevent against session fixation attacks.
    // This is called before hook_user in case one of those functions fails
    // or incorrectly does a redirect which would leave the old session in place.
    drupal_session_regenerate();

    // Some client/servers, like XMLRPC, do not handle cookies, so
    // imitate it to make sess_read() function try to look for user,
    // instead of just loading anonymous user :).
    /*$session_name = session_name();
    if (!isset($_COOKIE[$session_name])) {
      $_COOKIE[$session_name] = $sid;
    }

    session_id($sid);*/

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

    if (!is_array($required_scope))
      $required_scope = explode(" ", $required_scope);
    $required_scope = array_unique(array_merge($required_scope, $hidden_scope));

    $scopes = $this->checkExtendedPermissions($required_scope, $authorized_scope);

    $extended_permissions = array();
    foreach ($scopes as $scope) {
      $result = db_query("SELECT os.sid, os.scope_id, os.hidden, os.bypass FROM  {oauth2_scopes} AS os  WHERE os.scope_id = :s ", array(':s' => $scope));
      $extended_permissions[] = $result;
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
    $result = db_query("SELECT os.scope_id FROM  {oauth2_scopes} AS os  WHERE os.hidden = 1 ");
    $scopes = array();
    foreach ($result as $record) {
      $scopes[] = $record['scope_id'];
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
    $result = db_query("SELECT scope_id FROM {oauth2_authorize} WHERE client_id = :s AND uid = :d", array(':s' => $client_id, ':d' => $uid));
    $scopes = array();
    foreach ($result as $record) {
      $scopes[] = $record['scope_id'];
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
      db_query("INSERT INTO {oauth2_authorize} (client_id, uid, scope_id) VALUES (:client_id, :uid, :scope_id)", array(':client_id' => $client_id, ':uid' => $uid, 'scope_id' => $scope_id));
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
      db_query("DELETE FROM {oauth2_authorize} WHERE client_id = :client_id AND uid = :uid AND scope_id = :scope_id", array(':client_id' => $client_id, ':uid' => $uid, ':scope_id' => $scope_id));
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
    if (!is_array($required_scope))
      $required_scope = explode(" ", $required_scope);

    if (!is_array($authorized_scope))
      $authorized_scope = explode(" ", $authorized_scope);

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
    // Purge tokens based on oauth_token if provided.
    if (isset($_REQUEST['oauth_token']) && !empty($_REQUEST['oauth_token'])) {
      $result = $this->getAccessToken($_REQUEST['oauth_token']);
      if ($result) {
        $sid = $result['sid'];
      }
    }

    // Purge tokens if session found.
    if ($sid) {
      if (!isset($_GET['redirect_uri'])) {
        // We hack $_REQUEST['redirect_uri'] so handle it custom logout.
        $result = db_query("SELECT oc.redirect_uri
          FROM {oauth2_access_tokens} AS oat
            INNER JOIN {oauth2_clients} AS oc ON oat.client_id = oc.client_id
          WHERE oat.sid = :sid ", array(':sid' =>$sid));
        $record = $result->fetchObject();
          $_GET['redirect_uri'] = $record['redirect_uri'];
        
      }

      // Expirse all tokens base on this session_id.
      db_query("DELETE FROM {oauth2_access_tokens} WHERE sid = :sid", array(':sid' => $sid));
      db_query("DELETE FROM {oauth2_auth_codes} WHERE sid = :sid", array(':sid' => $sid));
      db_query("DELETE FROM {oauth2_refresh_tokens} WHERE sid = :sid", array(':sid' => $sid));

      // Also manually destroy user session.
      if (session_id($sid) != $sid) {
        session_start();
        session_id($sid);
      }

      return session_destroy();
    }

    return FALSE;
  }

  
  /**
   * Overrides OAuth2:grantAccessToken()
   */
  public function grantAccessToken() { global $user; error_log($user->uid);
    $filters = array(
        "grant_type" => array("filter" => FILTER_VALIDATE_REGEXP, "options" => array("regexp" => OAUTH2_GRANT_TYPE_REGEXP), "flags" => FILTER_REQUIRE_SCALAR),
        "scope" => array("flags" => FILTER_REQUIRE_SCALAR),
        "code" => array("flags" => FILTER_REQUIRE_SCALAR),
        "redirect_uri" => array("filter" => FILTER_SANITIZE_URL),
        "username" => array("flags" => FILTER_REQUIRE_SCALAR),
        "password" => array("flags" => FILTER_REQUIRE_SCALAR),
        "assertion_type" => array("flags" => FILTER_REQUIRE_SCALAR),
        "assertion" => array("flags" => FILTER_REQUIRE_SCALAR),
        "refresh_token" => array("flags" => FILTER_REQUIRE_SCALAR),
    );
  
    $input = filter_input_array(INPUT_POST, $filters);
  
    // Grant Type must be specified.
    if (!$input["grant_type"])
      $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_REQUEST, 'Invalid grant_type parameter or parameter missing');
  
    // Make sure we've implemented the requested grant type
    if (!in_array($input["grant_type"], $this->getSupportedGrantTypes()))
      $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_UNSUPPORTED_GRANT_TYPE);
  
    // Authorize the client
    $client = $this->getClientCredentials();
  
    if ($this->checkClientCredentials($client[0], $client[1]) === FALSE)
      $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_CLIENT);
  
    if (!$this->checkRestrictedGrantType($client[0], $input["grant_type"]))
      $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_UNAUTHORIZED_CLIENT);
  
    // Do the granting
    switch ($input["grant_type"]) {
      case OAUTH2_GRANT_TYPE_AUTH_CODE:
        if (!$input["code"] || !$input["redirect_uri"])
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_REQUEST);
  
        $stored = $this->getAuthCode($input["code"]);
  
        // Ensure that the input uri starts with the stored uri
        if ($stored === NULL || (strcasecmp(substr($input["redirect_uri"], 0, strlen($stored["redirect_uri"])), $stored["redirect_uri"]) !== 0) || $client[0] != $stored["client_id"])
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_GRANT);
  
        if ($stored["expires"] < time())
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_EXPIRED_TOKEN);
  
        break;
      case OAUTH2_GRANT_TYPE_USER_CREDENTIALS:
        if (!$input["username"] || !$input["password"])
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_REQUEST, 'Missing parameters. "username" and "password" required');
  
        $stored = $this->checkUserCredentials($client[0], $input["username"], $input["password"]);
  
        if ($stored === FALSE)
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_GRANT);
  
        break;
      case OAUTH2_GRANT_TYPE_ASSERTION:
        if (!$input["assertion_type"] || !$input["assertion"])
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_REQUEST);
  
        $stored = $this->checkAssertion($client[0], $input["assertion_type"], $input["assertion"]);
  
        if ($stored === FALSE)
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_GRANT);
  
        break;
      case OAUTH2_GRANT_TYPE_REFRESH_TOKEN:
        if (!$input["refresh_token"])
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_REQUEST, 'No "refresh_token" parameter found');
  
        $stored = $this->getRefreshToken($input["refresh_token"]);
  
        if ($stored === NULL || $client[0] != $stored["client_id"])
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_GRANT);
  
        if ($stored["expires"] < time())
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_EXPIRED_TOKEN);
  
        // store the refresh token locally so we can delete it when a new refresh token is generated
        $this->setVariable('_old_refresh_token', $stored["token"]);
  
        break;
      case OAUTH2_GRANT_TYPE_NONE:
        $stored = $this->checkNoneAccess($client[0]);
  
        if ($stored === FALSE)
          $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_REQUEST);
    }
  
    // Check scope, if provided
    if ($input["scope"] && (!is_array($stored) || !isset($stored["scope"]) || !$this->checkScope($input["scope"], $stored["scope"])))
      $this->errorJsonResponse(OAUTH2_HTTP_BAD_REQUEST, OAUTH2_ERROR_INVALID_SCOPE);
  
    if (!$input["scope"])
      $input["scope"] = NULL;
  
    $token = $this->createAccessToken($client[0], $input["scope"]);
  
    $this->sendJsonHeaders();
    echo json_encode($token);
  }
  
  /**
   * Implements OAuth2::checkClientCredentials().
   */
  public function checkClientCredentials($client_id, $client_secret = NULL) {
    $result = db_query("SELECT oc.client_secret FROM {oauth2_clients} AS oc WHERE oc.client_id = :client_id ", array(':client_id' => $client_id));
    $record_client_secret = $result->fetchField();
    return ($client_secret !== NULL && $record_client_secret == $client_secret) ? TRUE : FALSE;
  }

  /**
   * Implements OAuth2::getRedirectUri().
   */
  public function getRedirectUri($client_id) {
    $result = db_query("SELECT oc.redirect_uri FROM {oauth2_clients} AS oc  WHERE oc.client_id = :client_id ", array(':client_id' => $client_id ));
    $record_redirect_uri = $result->fetchField();
    return $record_redirect_uri !== FALSE ? $record_redirect_uri : NULL;
  }

  /**
   * Implements OAuth2::getAccessToken().
   */
  public function getAccessToken($oauth_token) {
    $result = db_query("SELECT oat.client_id, oat.sid, oat.expires, oat.scope, u.uid, u.name, u.mail
      FROM {oauth2_access_tokens} AS oat
        INNER JOIN {users} AS u ON oat.uid = u.uid
      WHERE oat.oauth_token = :s", array(':s' => $oauth_token));
    return $result->fetchAssoc();
  }

  /**
   * Implements OAuth2::setAccessToken().
   */
  public function setAccessToken($oauth_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return db_query("INSERT INTO {oauth2_access_tokens} (oauth_token, client_id, expires, scope, uid, sid) VALUES (:oauth_token, :client_id, :expires, :scope, :uid, :sid)", array(':oauth_token' => $oauth_token, ':client_id' => $client_id, ':expires' => $expires, ':scope' => $scope, ':uid' => $uid, ':sid' => $sid));
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
    $result = db_query("SELECT os.scope_id FROM {oauth2_scopes} AS os ");
    $scope_ids = array();
    foreach ($result as $record) {
      $scope_ids[] = $record['scope_id'];
    }
    return $scope_ids;
  }

  /**
   * Overrides OAuth2::getAuthCode().
   */
  public function getAuthCode($code) {
    $result = db_query("SELECT oac.client_id, oac.sid, oac.redirect_uri, oac.expires, oac.scope, u.uid, u.name
      FROM {oauth2_auth_codes} AS oac
        INNER JOIN {users} AS u ON oac.uid = u.uid
      WHERE oac.code = :code", array(':code' => $code));
    return $result->fetchAssoc();
  }

  /**
   * Overrides OAuth2::setAuthCode().
   */
  public function setAuthCode($code, $client_id, $redirect_uri, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return db_query("INSERT INTO {oauth2_auth_codes} (code, client_id, redirect_uri, expires, scope, uid, sid) VALUES (:code, :client_id, :redirect_uri, :expires, :scope, :uid, :sid)", array(':code' => $code, ':client_id' => $client_id, ':redirect_uri' => $redirect_uri, ':expires' => $expires, ':scope' => $scope, ':uid' => $uid, ':sid' => $sid));
  }

  /**
   * Overrides OAuth2::checkUserCredentials().
   */
  public function checkUserCredentials($client_id, $username, $password) {
    // Clone from user_service_login().
    $uid = user_authenticate($username, $password);

    $result = NULL;
    if ($uid) {
      $account = user_load($uid);
      // Regenerate the session ID to prevent against session fixation attacks.
      drupal_session_regenerate();

      $array = array();
      user_module_invoke('login', $array, $account);

      $result = array(
        'uid' => $account->uid,
        'name' => $account->name,
        'sid' => session_id(),
        'scope' => trim(implode(' ', $this->getAuthorizedScopes($client_id, $account->uid))),
      );
    }

    return $result ? $result : FALSE;
  }

  /**
   * Overrides OAuth2::getRefreshToken().
   */
  public function getRefreshToken($refresh_token) {
    $result = db_query("SELECT ort.client_id, ort.sid, ort.expires, ort.scope, u.uid, u.name
      FROM {oauth2_refresh_tokens} AS ort
        INNER JOIN {users} AS u ON ort.uid = u.uid
      WHERE ort.refresh_token = :token", array(':token' => $refresh_token));
    return $result->fetchAssoc();
  }

  /**
   * Overrides OAuth2::setRefreshToken().
   */
  public function setRefreshToken($refresh_token, $client_id, $expires, $scope = '', $uid = 0, $sid = '') {
    global $user;
    $uid = ($uid == 0 && $user->uid != 0) ? $user->uid : $uid;
    $sid = ($sid == '' && session_id()) ? session_id() : $sid;
    return db_query("INSERT INTO {oauth2_refresh_tokens} (refresh_token, client_id, expires, scope, uid, sid) VALUES (:refresh_token, :client_id, :expires, :scope, :uid, :sid)", array(':refresh_token' => $refresh_token, ':client_id' => $client_id, ':expires' => $expires, ':scope' => $scope, ':uid' => $uid, ':sid' => $sid));
  }

  /**
   * Overrides OAuth2::unsetRefreshToken().
   */
  public function unsetRefreshToken($refresh_token) {
    return db_query("DELETE FROM {oauth2_refresh_tokens} WHERE refresh_token = :s", array(':s' => $refresh_token));
  }
}
