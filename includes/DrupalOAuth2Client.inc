<?php

/**
 * @file
 * Drupal OAuth2.0 client library.
 */

/**
 * Provides access to the Drupal OAuth2.0 Platform.
 *
 * @author Edison Wong <hswong3i@pantarei-design.com>
 */
class DrupalOAuth2Client extends OAuth2Client {

  /**
   * Overrides OAuth2Client::__construct().
   */
  public function __construct($config = array()) {
    global $user;
  
    parent::__construct($config);
  
    $oauth2_mode = variable_get('oauth2_provider_mode');
    //only for client mode and no need to load if already logged in
    if ($oauth2_mode == 'DrupalOAuth2Client' && (empty($user->uid))) {

      // devel_execute_block_form also submit execute code in $_REQUEST['code'].
      if (!isset($_REQUEST['form_id']) || $_REQUEST['form_id'] !== 'devel_execute_block_form') {

        $access_token = $this->getAccessToken();
        //error_log('access token: '.$access_token);
        if (!empty($access_token)) {
          $result = $this->api('/oauth2/getAccessToken', 'POST', array(
              'oauth_token' => $access_token,
          ));
          if ($result) {
            $this->loadUserCredentials($result['name'], $result['mail']);
          }
        }
      }
  
    }
  }
  
  /**
   * Expires all OAuth2.0 related tokens based on sid.
   *
   * @return
   *   TRUE if successful, and FALSE if it isn't.
   */
  public function expireSession() {

    // we won't know the server session id
    $params = array(
      'sid' => NULL,
      'oauth_token' => $this->getAccessToken(),
    );

    // Purge tokens based on oauth_token if provided.
    /*if (isset($_REQUEST['oauth_token']) && !empty($_REQUEST['oauth_token'])) {
      $result = $this->getAccessToken($_REQUEST['oauth_token']);
      if ($result) {
        $params = array(
          'sid' => $result['sid'],
          'oauth_token' => $_REQUEST['oauth_token'],
        );
      }
    }*/
    

    return $this->api('/oauth2/expireSession', 'POST', $params);
  }
  /**
   * Load the user based on uid and name, assign with corresponding session;
   * or create dummy user on-the-fly.
   *
   * @param $name
   *   The user name.
   * @param $mail
   *   The user email.
   *
   * @return
   *   The user object after successful login.
   */
  protected function loadUserCredentials($name, $mail) {
    global $user;

    $root_user = user_load(1);
    if ($name == $root_user->name) {
      return $user;
    }
    user_external_login_register($name,'oauth2');
    
    /*if (!empty($user->uid)) {
      return $user;
    }
  
    if (isset($name) && !($account = user_load_by_name($name))) {
      // We hack and create a dummy user placeholder if it is exist in
      // token tables but not yet a real user account, typically
      // happened when server running in proxy mode. Can't use
      // drupal_write_record() as uid is serial.
      
      $user_array = array(
          'name'   => $name,
          'pass'   => user_password(20),
          'mail'   => $mail,
          'init'   => $mail,
          'status' => 1,
      );
      $user = user_save(NULL, $user_array);
      if ($user === FALSE) {
        watchdog('user', 'failed to create user %name.', array('%name' => $name));
        return FALSE;
      }
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
    drupal_session_regenerate();*/
  

  
    return $user;
  }
  /**
   * Get a Login URL for use with redirects. By default, full page redirect is
   * assumed. If you are using the generated URL with a window.open() call in
   * JavaScript, you can pass in display = popup as part of the $params.
   *
   * @param $params
   *   Provide custom parameters.
   *
   * @return
   *   The URL for the login flow.
   */
  public function getLoginUri($params = array()) {
    return $this->getUri(
      $this->getVariable('base_uri') . 'user',
      array_merge(array(
        'response_type' => 'code',
        'client_id' => $this->getVariable('client_id'),
        'redirect_uri' => $this->getCurrentUri(),
      ), $params)
    );
  }

  /**
   * Get a Logout URL suitable for use with redirects.
   *
   * @param $params
   *   Provide custom parameters.
   *
   * @return
   *   The URL for the logout flow.
   */
  public function getLogoutUri($params = array()) {
    return $this->getUri(
      $this->getVariable('base_uri') . 'logout',
      array_merge(array(
        'oauth_token' => $this->getAccessToken(),
        'redirect_uri' => $this->getCurrentUri(),
      ), $params)
    );
  }
}
