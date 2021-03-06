package com.feiniu.subAccount.util;

import com.thoughtworks.xstream.XStream;

/**
 * Created by morningking on 2014/12/24.
 */
public class XMLUtils {
    public static final <T> T getBean(String xml, Class<T> clazz) {
        XStream xStream = new XStream();
        xStream.alias(clazz.getSimpleName().toLowerCase(), clazz);
        return (T) xStream.fromXML(xml);
    }
}
