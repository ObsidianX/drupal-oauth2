function getParameterByName(name) {
    var match = RegExp('[?&]' + name + '=([^&]*)')
                    .exec(window.location.search);
    if (!match) {
      return false;
    }
    return decodeURIComponent(match[1].replace(/\+/g, ' '));
}

jQuery(document).ready(function ($) {
  //if inside iframe
  if (window != parent && getParameterByName('code') != false) {
    parent.location.reload();
    return;
  }

  
  var ifrm = document.createElement("IFRAME");
  ifrm.style.width = 640+"px";
  ifrm.style.height = 480+"px";
  ifrm.style.display = 'none';
  ifrm.id = 'oauth2_iframe';
  
  document.body.appendChild(ifrm);
  //console.debug(Drupal.settings.oauth2.url);
  if (!Drupal.settings.oauth2.logged_in && Drupal.settings.oauth2.auto_login_enabled) {
     $("#oauth2_iframe").attr('src',Drupal.settings.oauth2.url+Drupal.settings.oauth2.authorize_uri+'?response_type=code&client_id='+Drupal.settings.oauth2.client_id+'&redirect_uri='+Drupal.settings.oauth2.redirect_uri);
  }
  
  $('#oauth2_login_iframe').click(function() {

     var iframe =  $("#oauth2_iframe").detach();
       $(this).parent().append(iframe);
       if (!$(iframe).attr('src')){
           $(iframe).attr('src',Drupal.settings.oauth2.url+Drupal.settings.oauth2.authorize_uri+'?response_type=code&client_id='+Drupal.settings.oauth2.client_id+'&redirect_uri='+Drupal.settings.oauth2.redirect_uri);
       }
       $(iframe).show();
  });
});