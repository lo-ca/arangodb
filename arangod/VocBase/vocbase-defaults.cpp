////////////////////////////////////////////////////////////////////////////////
/// @brief vocbase database defaults
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2014 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Copyright 2014, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2011-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "vocbase-defaults.h"
#include "Basics/json.h"
#include "VocBase/vocbase.h"

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief apply default settings
////////////////////////////////////////////////////////////////////////////////

void TRI_vocbase_defaults_t::applyToVocBase (TRI_vocbase_t* vocbase) const {
  vocbase->_settings.defaultMaximalSize               = defaultMaximalSize;
  vocbase->_settings.defaultWaitForSync               = defaultWaitForSync;
  vocbase->_settings.requireAuthentication            = requireAuthentication;
  vocbase->_settings.requireAuthenticationUnixSockets = requireAuthenticationUnixSockets;
  vocbase->_settings.authenticateSystemOnly           = authenticateSystemOnly;
  vocbase->_settings.forceSyncProperties              = forceSyncProperties;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert defaults into a VelocyPack array
////////////////////////////////////////////////////////////////////////////////

void TRI_vocbase_defaults_t::toVelocyPack(VPackBuilder& builder) const {
  TRI_ASSERT(! builder.isClosed());
  
  builder.add("waitForSync", VPackValue(defaultWaitForSync));
  builder.add("requireAuthentication", VPackValue(requireAuthentication));
  builder.add("requireAuthenticationUnixSockets", VPackValue(requireAuthenticationUnixSockets));
  builder.add("authenticateSystemOnly", VPackValue(authenticateSystemOnly));
  builder.add("forceSyncProperties", VPackValue(forceSyncProperties));
  builder.add("defaultMaximalSize", VPackValue(defaultMaximalSize));
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert defaults into a VelocyPack array
////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<VPackBuilder> TRI_vocbase_defaults_t::toVelocyPack() const {
  std::shared_ptr<VPackBuilder> builder(new VPackBuilder());
  builder->openArray();
  toVelocyPack(*builder);
  builder->close();
  return builder;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief enhance defaults with data from JSON
////////////////////////////////////////////////////////////////////////////////

void TRI_FromJsonVocBaseDefaults (TRI_vocbase_defaults_t* defaults,
                                  VPackSlice const& slice) {
  if (! slice.isObject()) {
    return;
  }
  VPackSlice optionSlice = slice.get("waitForSync");
  if (optionSlice.isBoolean()) {
    defaults->defaultWaitForSync = optionSlice.getBool();
  }

  optionSlice = slice.get("requireAuthentication");
  if (optionSlice.isBoolean()) {
    defaults->requireAuthentication = optionSlice.getBool();
  }

  optionSlice = slice.get("requireAuthenticationUnixSockets");
  if (optionSlice.isBoolean()) {
    defaults->requireAuthenticationUnixSockets = optionSlice.getBool();
  }

  optionSlice = slice.get("authenticateSystemOnly");
  if (optionSlice.isBoolean()) {
    defaults->authenticateSystemOnly = optionSlice.getBool();
  }
  
  optionSlice = slice.get("forceSyncProperties");
  if (optionSlice.isBoolean()) {
    defaults->forceSyncProperties = optionSlice.getBool();
  }

  optionSlice = slice.get("forceSyncProperties");
  if (optionSlice.isNumber()) {
    defaults->forceSyncProperties = optionSlice.getNumericValue<TRI_voc_size_t>();
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
