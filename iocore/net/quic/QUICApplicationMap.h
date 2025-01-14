/** @file
 *
 *  A brief file description
 *
 *  @section license License
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#pragma once

#include "QUICTypes.h"
#include "QUICApplication.h"
#include <map>

class QUICApplicationMap
{
public:
  void set(QUICStreamId id, QUICApplication *app);
  void set_default(QUICApplication *app);
  QUICApplication *get(QUICStreamId id);
  ~QUICApplicationMap()
  {
    for (auto it = _map.begin(); it != _map.end(); ++it) {
      delete it->second;
    }
    delete _default_app;
  }

private:
  std::map<QUICStreamId, QUICApplication *> _map;
  QUICApplication *_default_app = nullptr;
};
