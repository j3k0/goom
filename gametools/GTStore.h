//
//  GTStore.h
//  Birdy Nam Nam
//
//  Created by Jean-Christophe Hoelt on 8/29/11.
//  Copyright 2011 Fovea.cc. All rights reserved.
//

#ifndef GTStore_H
#define GTStore_H

namespace gametools {
    
class GTTransactionListener {
public:
    virtual ~GTTransactionListener() {}

    virtual void onTransactionSucceeded(const char *productId) = 0;
    virtual void onTransactionFailed(const char *productId) = 0;
    virtual void onTransactionCancelled(const char *productId) = 0;
};

class GTStore {
public:
    virtual ~GTStore() {}
    
    // Preload the store.
    // productIds is a NULL terminated list of Product IDs.
    // listeners array contains listeners for each product ID, they can be NULL
   virtual void loadStore(const char **productIds, GTTransactionListener **listeners) {}
    
    // Returns true iff product has been loaded successfully 
    virtual bool isLoaded(const char *productId) { return false; }
    
    // Returns the localized title of an item from its id.
    virtual const char *getTitle(const char *productId) { return ""; }
    
    // Returns the localized title of an item from its id.
    virtual const char *getDescription(const char *productId) { return ""; }
    
    // Returns the localized price of an item from its id.
    virtual const char *getPrice(const char *productId) { return ""; }

    // Returns true iff the user have the ability to buy.
    virtual bool canMakePurchase() { return false; }
    
    // Purchase an item.
    // Listener is optional, but make sure one has been defined in loadStore()
    virtual void purchase(const char *productId, GTTransactionListener *listener = NULL) {}
};
    
}

#endif