// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('cr.icon', function() {
  /**
   * @return {!Array<number>} The scale factors supported by this platform for
   *     webui resources.
   */
  function getSupportedScaleFactors() {
    const supportedScaleFactors = [];
    if (!cr.isIOS) {
      // This matches the code in ResourceBundle::InitSharedInstance() that
      // supports SCALE_FACTOR_100P on all non-iOS platforms.
      supportedScaleFactors.push(1);
    }
    if (cr.isMac || cr.isChromeOS || cr.isWindows || cr.isLinux) {
      // All desktop platforms support zooming which also updates the renderer's
      // device scale factors (a.k.a devicePixelRatio), and these platforms have
      // high DPI assets for 2x.  Let the renderer pick the closest image for
      // the current device scale factor.
      supportedScaleFactors.push(2);
    } else {
      // For other platforms that use fixed device scale factor, use
      // the window's device pixel ratio.
      // TODO(oshima): Investigate corresponding to
      // ResourceBundle::InitSharedInstance() more closely.
      supportedScaleFactors.push(window.devicePixelRatio);
    }
    return supportedScaleFactors;
  }

  /**
   * Generates a CSS url string.
   * @param {string} s The URL to generate the CSS url for.
   * @return {string} The CSS url string.
   */
  function getUrlForCss(s) {
    // http://www.w3.org/TR/css3-values/#uris
    // Parentheses, commas, whitespace characters, single quotes (') and double
    // quotes (") appearing in a URI must be escaped with a backslash
    let s2 = s.replace(/(\(|\)|\,|\s|\'|\"|\\)/g, '\\$1');
    // WebKit has a bug when it comes to URLs that end with \
    // https://bugs.webkit.org/show_bug.cgi?id=28885
    if (/\\\\$/.test(s2)) {
      // Add a space to work around the WebKit bug.
      s2 += ' ';
    }
    return 'url("' + s2 + '")';
  }

  /**
   * A URL for the filetype icon for |filePath|. OS and theme dependent.
   * @param {string} filePath
   * @return {string}
   */
  function getFileIconUrl(filePath) {
    const url = new URL('chrome://fileicon/');
    url.searchParams.set('path', filePath);
    url.searchParams.set('scale', window.devicePixelRatio + 'x');
    return url.toString();
  }

  /**
   * Generates a CSS -webkit-image-set for a chrome:// url.
   * An entry in the image set is added for each of getSupportedScaleFactors().
   * The scale-factor-specific url is generated by replacing the first instance
   * of 'scalefactor' in |path| with the numeric scale factor.
   *
   * @param {string} path The URL to generate an image set for.
   *     'scalefactor' should be a substring of |path|.
   * @return {string} The CSS -webkit-image-set.
   */
  function getImageSet(path) {
    const supportedScaleFactors = getSupportedScaleFactors();

    const replaceStartIndex = path.indexOf('SCALEFACTOR');
    if (replaceStartIndex < 0) {
      return getUrlForCss(path);
    }

    let s = '';
    for (let i = 0; i < supportedScaleFactors.length; ++i) {
      const scaleFactor = supportedScaleFactors[i];
      const pathWithScaleFactor = path.substr(0, replaceStartIndex) +
          scaleFactor + path.substr(replaceStartIndex + 'scalefactor'.length);

      s += getUrlForCss(pathWithScaleFactor) + ' ' + scaleFactor + 'x';

      if (i != supportedScaleFactors.length - 1) {
        s += ', ';
      }
    }
    return '-webkit-image-set(' + s + ')';
  }

  /**
   * Returns the URL of the image, or an image set of URLs for the provided
   * path.  Resources in chrome://theme have multiple supported scale factors.
   *
   * @param {string} path The path of the image.
   * @return {string} The url, or an image set of URLs.
   */
  function getImage(path) {
    const chromeThemePath = 'chrome://theme';
    const isChromeThemeUrl =
        (path.slice(0, chromeThemePath.length) == chromeThemePath);
    return isChromeThemeUrl ? getImageSet(path + '@SCALEFACTORx') :
                              getUrlForCss(path);
  }

  /**
   * A regular expression for identifying favicon URLs.
   * @const {!RegExp}
   */
  const FAVICON_URL_REGEX = /\.(ico|png)$/i;

  /**
   * Creates a CSS -webkit-image-set for a favicon request.
   *
   * @param {string} url Either the URL of the original page or of the favicon
   *     itself.
   * @param {boolean} isSyncedUrlForHistoryUi Should be set to true only if the
   *     caller is an UI aimed at displaying user history, and the requested url
   *     is known to be present in Chrome sync data.
   *
   * @return {string} -webkit-image-set for the favicon.
   */
  function getFavicon(url, isSyncedUrlForHistoryUi) {
    const isIconUrl = FAVICON_URL_REGEX.test(url);
    // Note: Literal strings used below must match those in the description of
    // chrome://favicon2 format in components/favicon_base/favicon_url_parser.h.
    let path = 'chrome://favicon2/?size=16&scale_factor=SCALEFACTORx' +
        '&url_type=' + (isIconUrl ? 'icon_url' : 'page_url') +
        '&url=' + encodeURIComponent(url);
    if (!isIconUrl) {
      path += '&allow_google_server_fallback=' +
          (isSyncedUrlForHistoryUi ? '1' : '0');
    }
    return getImageSet(path);
  }

  return {
    getFavicon: getFavicon,
    getFileIconUrl: getFileIconUrl,
    getImage: getImage,
    getUrlForCss: getUrlForCss,
  };
});
