// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('welcome', function() {
  /**
   * NuxGoogleAppsSelections enum.
   * These values are persisted to logs and should not be renumbered or
   * re-used.
   * See tools/metrics/histograms/enums.xml.
   * @enum {number}
   */
  const NuxGoogleAppsSelections = {
    GMAIL_DEPRECATED: 0,
    YOU_TUBE: 1,
    MAPS: 2,
    TRANSLATE: 3,
    NEWS: 4,
    CHROME_WEB_STORE: 5,
  };

  /** @interface */
  class GoogleAppProxy {
    /**
     * Google app IDs are local to the list of Google apps, so their icon must
     * be cached by the handler that provided the IDs.
     * @param {number} appId
     */
    cacheBookmarkIcon(appId) {}

    /**
     * Returns a promise for an array of Google apps.
     * @return {!Promise<!Array<!welcome.BookmarkListItem>>}
     */
    getAppList() {}

    /**
     * @param {number} providerId This should match one of the histogram enum
     *     value for NuxGoogleAppsSelections.
     */
    recordProviderSelected(providerId) {}
  }

  /** @implements {welcome.GoogleAppProxy} */
  class GoogleAppProxyImpl {
    /** @override */
    cacheBookmarkIcon(appId) {
      chrome.send('cacheGoogleAppIcon', [appId]);
    }

    /** @override */
    getAppList() {
      return cr.sendWithPromise('getGoogleAppsList');
    }

    /** @override */
    recordProviderSelected(providerId) {
      chrome.metricsPrivate.recordEnumerationValue(
          'FirstRun.NewUserExperience.GoogleAppsSelection', providerId,
          Object.keys(NuxGoogleAppsSelections).length);
    }
  }

  cr.addSingletonGetter(GoogleAppProxyImpl);

  return {
    GoogleAppProxy: GoogleAppProxy,
    GoogleAppProxyImpl: GoogleAppProxyImpl,
  };
});
