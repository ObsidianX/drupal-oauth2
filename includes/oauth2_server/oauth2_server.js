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
  }
});