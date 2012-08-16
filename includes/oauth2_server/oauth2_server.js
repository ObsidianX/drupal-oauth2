function getParameterByName(name) {
    var match = RegExp('[?&]' + name + '=([^&]*)')
                    .exec(window.location.search);
    if (!match) {
      return false;
    }
    return decodeURIComponent(match[1].replace(/\+/g, ' '));
}

jQuery(document).ready(function () {
  //if inside iframe
  if (window != parent && getParameterByName('code') != false) {
    parent.location.reload();
    return;
  }
  //
  //http://server7.oauth2test.pantarei-design.com/oauth2/authorize?response_type=code&client_id=9eb6db0c37e2067cbaa4b9dacf1c3922&redirect_uri=http://dev.client7.oauth2.pantarei-design.com
  //http://server7.oauth2test.pantarei-design.com/oauth2/authorize?response_type=code&client_id=2a0d4c44690fbae5c94871686fb9408d&redirect_uri=http://dev.client7.oauth2.pantarei-design.com/
  //console.debug(Drupal.settings.oauth2.url);
  if (!Drupal.settings.oauth2.logged_in) {
  var ifrm = document.createElement("IFRAME");
  ifrm.setAttribute("src", Drupal.settings.oauth2.url+Drupal.settings.oauth2.authorize_uri+'?response_type=code&client_id='+Drupal.settings.oauth2.client_id+'&redirect_uri='+Drupal.settings.oauth2.redirect_uri);
  ifrm.style.width = 640+"px";
  ifrm.style.height = 480+"px";
  //ifrm.style.display = 'none';
  document.body.appendChild(ifrm);
  }
});